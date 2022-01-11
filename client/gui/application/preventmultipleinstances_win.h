#ifndef PREVENTMULTIPLEINSTANCES_WIN_H
#define PREVENTMULTIPLEINSTANCES_WIN_H

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

#endif // PREVENTMULTIPLEINSTANCES_WIN_H
