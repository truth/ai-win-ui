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
