#pragma once

#include <Windows.h>

namespace wsl
{

class Win32Handle
{
public:
    explicit
    Win32Handle(HANDLE hWin32Handle = nullptr) : win32Handle_(hWin32Handle) {}

    ~Win32Handle() {
        closeHandle();
    }

    bool isValid() const {
        return ((win32Handle_ != nullptr) && (win32Handle_ != INVALID_HANDLE_VALUE));
    }

    HANDLE getHandle() const { return win32Handle_; }
    PHANDLE data() { return &win32Handle_; }

    void setHandle(HANDLE hNewHandle);
    void closeHandle();
    HANDLE release();
    DWORD wait(DWORD dwTimeout, BOOL bAlertable = FALSE) const;

    bool isSignaled() const;

private:
    HANDLE win32Handle_;
};

inline void Win32Handle::closeHandle()
{
    if (isValid()) {
        ::CloseHandle(win32Handle_);
        win32Handle_ = nullptr;
    }
}

inline HANDLE Win32Handle::release()
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
