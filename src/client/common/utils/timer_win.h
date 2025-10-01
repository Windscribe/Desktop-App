#pragma once

#include "utils/win32handle.h"

// Helper functions for the waitable Windows timer
namespace timer_win {

HANDLE createTimer(int timeout, bool singleShot, PTIMERAPCROUTINE completionRoutine, LPVOID argToCompletionRoutine);
void cancelTimer(wsl::Win32Handle &timer);


} // namespace timer_win
