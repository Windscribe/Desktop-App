#pragma once

#include <mutex>
#include <Windows.h>

// thread safe wrapper for Windows Filtering Platform handle

class FwpmWrapper
{
public:
    FwpmWrapper();
    ~FwpmWrapper();

    HANDLE getHandleAndLock();
    void unlock();

    bool initialize();
    void release();

    bool beginTransaction();
    bool endTransaction();

private:
    HANDLE engineHandle_ = nullptr;
    std::mutex mutex_;
};
