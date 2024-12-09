#pragma once

#include <thread>
#include <mutex>

class InstallerBase
{
public:
    InstallerBase();
    virtual ~InstallerBase();

    void start();
    void cancel();
    void waitForCompletion();
    void launchApp();

protected:
    std::thread extract_thread;
    std::mutex mutex_;

    bool isCanceled_;

    virtual void startImpl() = 0;
    virtual void executionImpl() = 0;
    virtual void launchAppImpl() = 0;
};
