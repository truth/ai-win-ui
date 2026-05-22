# Debugging Skills & Experience

## 1. Win32 API IME Infinite Recursion (STATUS_STACK_OVERFLOW)
**Symptom:**
Application crashes immediately on startup or when IME context changes, returning a mysterious `STATUS_FATAL_USER_CALLBACK_EXCEPTION` (`0xC000041D`).

**Diagnosis:**
This exception typically implies that an unhandled error occurred within a window procedure callback (`WndProc`). By wrapping the window procedure logic in a structural exception handler (`__try / __except (EXCEPTION_EXECUTE_HANDLER)`) and printing `GetExceptionCode()`, it revealed the true underlying error: `0xC00000FD` (Stack Overflow), which was triggered during message `0x282` (`WM_IME_NOTIFY`).

**Root Cause:**
Inside the `WM_IME_NOTIFY` message handler, the code attempted to update the IME window position by calling `ImmSetCompositionWindow()`. However, calling `ImmSetCompositionWindow()` inherently triggers an `IMN_SETCOMPOSITIONWINDOW` notification, which the OS dispatches to the application as *another* `WM_IME_NOTIFY` message. This circular dependency creates an infinite recursion that rapidly exhausts the thread stack.

**Solution:**
Never unconditionally update the composition window (via `ImmSetCompositionWindow`) in response to `WM_IME_NOTIFY`. Instead, restrict IME position updates to `WM_IME_STARTCOMPOSITION` and `WM_IME_COMPOSITION` messages, which do not circularly trigger themselves.

## 2. Skia Text Rendering (O(N^2) Latency from Cache Misses)
**Symptom:**
Rendering text strings with Chinese/Japanese/Korean (CJK) characters alongside English characters caused massive memory allocations and thread lockups, drastically slowing down or crashing the UI.

**Diagnosis & Root Cause:**
The font fallback logic (`IterateTextRuns`) strictly compared the matched character's font against the system `defaultTypeface` rather than the `currentTypeface` being used for the sequence. Consequently, if consecutive CJK characters fell back to the same secondary font (e.g., "Microsoft YaHei"), they failed the equality check because they were compared against the English default typeface. This caused the text engine to break the string into single-character runs, issuing thousands of individual rendering draw calls and triggering $O(N^2)$ measurements.

**Solution:**
Ensure consecutive characters sharing the exact same matched fallback typeface are batched together by comparing `currentTypeface.get() == charTypeface.get()`. Additionally, caching resolved fallbacks in an `std::unordered_map` eliminates redundant OS-level font queries.

## 3. Debugging `STATUS_FATAL_USER_CALLBACK_EXCEPTION` without WinDbg
**Context:**
When a Win32 application crashes with `0xC000041D` (`STATUS_FATAL_USER_CALLBACK_EXCEPTION`), the OS is masking the true exception (such as Access Violation `0xC0000005` or Stack Overflow `0xC00000FD`). This happens because the exception originated inside a Windows callback (like `WndProc`) and crossed the user-mode callback boundary back into `USER32.dll` or `ntdll.dll`.

**Methodology:**
Instead of relying on heavy debuggers (like WinDbg/CDB) or complex minidump parsing to retrieve the nested exception record, you can quickly isolate the crash by injecting Structured Exception Handling (SEH) directly at the callback boundary:

1. **Wrap the Window Procedure in `__try / __except`**:
   Locate your window procedure (e.g., `WndProc` or `WndProcThunk`) and wrap the internal execution in a Microsoft-specific SEH block.
   ```cpp
   static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
       auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
       if (!app) return DefWindowProcW(hwnd, msg, wParam, lParam);

       __try {
           return app->HandleMessage(hwnd, msg, wParam, lParam);
       } __except (EXCEPTION_EXECUTE_HANDLER) {
           // Grab the true underlying exception code and the message that triggered it
           DWORD code = GetExceptionCode();
           FILE* f;
           if (fopen_s(&f, "seh_crash.log", "a") == 0) {
               fprintf(f, "SEH EXCEPTION: 0x%X during message 0x%X\n", code, msg);
               fclose(f);
           }
           std::exit(1);
       }
   }
   ```

2. **Trigger the Crash and Inspect the Log**:
   Run the application normally. The OS will execute the callback, and instead of swallowing the true exception into a generic `0xC000041D` dump, your `__except` block will catch it. 
   
   In our case, the log cleanly output `SEH EXCEPTION: 0xC00000FD during message 0x282`, immediately pointing to a Stack Overflow (`0xFD`) during `WM_IME_NOTIFY` (`0x282`), turning an hours-long blind debugging session into a 2-minute fix.

---

# Advanced C++ Windows Debugging Methodologies

Beyond SEH injection, modern C++ development on Windows relies on a robust set of diagnostic tools. Below is a curated list of advanced debugging methodologies every Windows C++ developer should master.

## 4. Memory Corruption Catching with Application Verifier & PageHeap
**Context:** 
Memory corruptions (Use-After-Free, Heap Buffer Overflow) often crash the application long after the actual memory violation occurred, making the crash dump point to innocent code.
**Methodology:**
- Enable **PageHeap** using `Application Verifier` (AppVerif.exe) or `gflags`.
- PageHeap places a non-accessible guard page immediately after every heap allocation. If your code writes out of bounds by even a single byte, the CPU triggers an immediate Access Violation (`0xC0000005`) exactly at the offending instruction.
- **Usage:** Run `gflags /p /enable YourApp.exe` and attach a debugger. The crash will now happen exactly where the bug is, not 10 minutes later.

## 5. Modern Memory Safety with AddressSanitizer (ASAN)
**Context:** 
Historically a Linux/Clang feature, ASAN is now natively integrated into MSVC and provides significantly faster and more comprehensive memory checks than PageHeap.
**Methodology:**
- Compile your project with `/fsanitize=address`.
- ASAN instruments memory allocations at compile-time and uses shadow memory to track the state of every byte. 
- It can catch Stack Out-Of-Bounds, Heap Out-Of-Bounds, Use-After-Free, and Use-After-Return with precise stack traces and minimal runtime overhead compared to PageHeap.

## 6. Post-Mortem Crash Analysis with WinDbg / CDB
**Context:**
When a crash happens on a user's machine, you only receive a `.dmp` (Minidump) file. Visual Studio can open these, but WinDbg is the ultimate source of truth.
**Methodology:**
- Open the `.dmp` file in WinDbg.
- Set symbol paths (e.g., `.sympath srv*C:\Symbols*https://msdl.microsoft.com/download/symbols;C:\path\to\your\pdb`).
- Run `!analyze -v`. WinDbg's automated analysis engine will walk the stack, identify the faulting instruction, cross-reference it with your PDBs, and explicitly tell you the exact file and line number that crashed.
- Use `~*k` to view all thread stacks to check for deadlocks.

## 7. Performance Profiling with ETW and WPA
**Context:** 
Your application isn't crashing, but the UI is lagging or experiencing micro-stutters (e.g., $O(N^2)$ layout measuring).
**Methodology:**
- **ETW (Event Tracing for Windows)** is the lowest-overhead telemetry system in Windows.
- Use `xperf` or Windows Performance Recorder (WPR) to record a trace (`.etl` file) while the lag occurs.
- Open the trace in **Windows Performance Analyzer (WPA)**.
- You can visualize CPU usage per thread, context switches, and identify exact functions causing high CPU latency via the precise call tree (Flame Graphs).

## 8. Dependency & System Interaction Debugging with Process Monitor (ProcMon)
**Context:** 
The application fails to start with "DLL not found", reads incorrect configurations, or fails to open resources silently.
**Methodology:**
- Run Sysinternals **Process Monitor**.
- Filter by `Process Name is YourApp.exe`.
- Watch the live stream of every single File System, Registry, and Network call the application makes.
- Look for `NAME NOT FOUND` or `ACCESS DENIED` results. This is the fastest way to debug "DLL Hell" (e.g., loading the wrong version of `zlib.dll` from `System32` instead of your local directory).

## 9. Automated Production Dumps (`MiniDumpWriteDump`)
**Context:** 
You need crash reports from production environments automatically without asking users to find `%LOCALAPPDATA%\CrashDumps`.
**Methodology:**
- Call `SetUnhandledExceptionFilter()` to register a top-level crash handler.
- Inside the handler, dynamically load `dbghelp.dll` and call `MiniDumpWriteDump()`.
- You can customize the dump type (e.g., `MiniDumpWithIndirectlyReferencedMemory`) to include just enough memory to see string values and object states without generating massive multi-gigabyte full heap dumps.
