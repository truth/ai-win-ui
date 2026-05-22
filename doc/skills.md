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
