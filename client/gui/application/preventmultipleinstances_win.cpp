#include "preventmultipleinstances_win.h"

#include <QtGlobal>
#include "utils/ws_assert.h"

PreventMultipleInstances_win::PreventMultipleInstances_win() : hMutexCurrentApp_(NULL)
{

}

PreventMultipleInstances_win::~PreventMultipleInstances_win()
{
    unlock();
}

bool PreventMultipleInstances_win::lock()
{
    WS_ASSERT(!hMutexCurrentApp_);
    if (hMutexCurrentApp_)
        return true;

    hMutexCurrentApp_ = CreateMutex((LPSECURITY_ATTRIBUTES)NULL, (BOOL)TRUE, (LPCTSTR)"WINDSCRIBE_APPLICATION");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutexCurrentApp_);
        hMutexCurrentApp_ = NULL;
        return false;
    }
    else
    {
        return true;
    }
}

void PreventMultipleInstances_win::unlock()
{
    if (hMutexCurrentApp_)
    {
        ReleaseMutex(hMutexCurrentApp_);
        CloseHandle(hMutexCurrentApp_);
        hMutexCurrentApp_ = NULL;
    }
}
