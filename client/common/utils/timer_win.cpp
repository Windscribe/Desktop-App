#include "timer_win.h"

namespace timer_win {

HANDLE createTimer(int timeout, bool singleShot, PTIMERAPCROUTINE completionRoutine, LPVOID argToCompletionRoutine)
{
    HANDLE hTimer = ::CreateWaitableTimer(NULL, FALSE, NULL);
    if (hTimer == NULL) {
        return NULL;
    }

    LARGE_INTEGER initialTimeout;
    initialTimeout.QuadPart = -(timeout * 10000);

    LONG period = (singleShot ? 0 : timeout);

    BOOL result = ::SetWaitableTimer(hTimer, &initialTimeout, period, completionRoutine, argToCompletionRoutine, FALSE);
    if (result == FALSE) {
        ::CloseHandle(hTimer);
        return NULL;
    }

    return hTimer;
}

void cancelTimer(wsl::Win32Handle &timer)
{
    if (timer.isValid()) {
        ::CancelWaitableTimer(timer.getHandle());
        timer.closeHandle();
    }
}

} // namespace timer_win
