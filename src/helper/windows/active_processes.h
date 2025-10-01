#pragma once

#include <Windows.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

class ActiveProcesses
{
public:
    static ActiveProcesses &instance()
    {
        static ActiveProcesses ap;
        return ap;
    }

    void release();

    // if bInitCall = true, then clear previuos saved data
    // else use previous saved PID data for optimize speed (reduce CPU usage by windscribe service)
    std::vector<std::wstring> getList();

private:
    ActiveProcesses();
    void clearSavedListProcesses();
    bool isModuleExists(TCHAR *szProcessNameWithPrefix);
    bool isSkipThisProcess(TCHAR *szProcessName);

    static VOID CALLBACK timerCallback(PVOID   lpParameter, BOOLEAN TimerOrWaitFired);

    enum { CLEAR_SAVED_LIST_PERIOD_MS = 20000};

    struct ProcessDescr
    {
        HANDLE hProcess;
        std::wstring module;
        bool bUsed;
    };

    HANDLE hTimer_;
    std::mutex mutex_;
    std::map<DWORD, ProcessDescr *> processes_;
};
