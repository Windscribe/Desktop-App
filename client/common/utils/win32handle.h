#pragma once

#include <Windows.h>

namespace wsl
{

class Win32Handle
{
public:
    explicit
    Win32Handle(HANDLE hWin32Handle = nullptr) : win32Handle_(hWin32Handle) {}

    ~Win32Handle(void) {
        closeHandle();
    }

    bool isValid(void) const {
        return ((win32Handle_ != nullptr) && (win32Handle_ != INVALID_HANDLE_VALUE));
    }

    HANDLE getHandle(void) const { return win32Handle_; }

    void setHandle(HANDLE hNewHandle);
    void closeHandle(void);
    HANDLE release(void);
    DWORD wait(DWORD dwTimeout, BOOL bAlertable = FALSE) const;

    bool isSignaled() const;

private:
    HANDLE win32Handle_;
};

inline void Win32Handle::closeHandle(void)
{
    if (isValid()) {
        ::CloseHandle(win32Handle_);
        win32Handle_ = nullptr;
    }
}

inline HANDLE Win32Handle::release(void)
{
    HANDLE hTemp = win32Handle_;
    win32Handle_ = nullptr;
    return hTemp;
}

inline void Win32Handle::setHandle(HANDLE hNewHandle)
{
    closeHandle();
    win32Handle_ = hNewHandle;
}

inline DWORD Win32Handle::wait(DWORD dwTimeout, BOOL bAlertable) const
{
    if (isValid()) {
        return ::WaitForSingleObjectEx(win32Handle_, dwTimeout, bAlertable);
    }

    return WAIT_FAILED;
}

inline bool Win32Handle::isSignaled() const
{
    return (wait(0, FALSE) == WAIT_OBJECT_0);
}

}
