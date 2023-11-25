#pragma once

#include <windows.h>

class PreventMultipleInstances_win
{
public:
    PreventMultipleInstances_win();
    ~PreventMultipleInstances_win();
    bool lock();
    void unlock();

private:
    HANDLE hMutexCurrentApp_;
};
