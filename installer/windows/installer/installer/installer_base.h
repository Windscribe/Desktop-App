#pragma once

#include <thread>
#include <mutex>

enum INSTALLER_CURRENT_STATE {
    STATE_INIT,
    STATE_EXTRACTING,
    STATE_CANCELED,
    STATE_FINISHED,
    STATE_ERROR,
    STATE_LAUNCHED,
    STATE_EXTRACTED,
};

enum INSTALLER_ERROR { ERROR_OTHER = 1, ERROR_PERMISSION, ERROR_KILL, ERROR_CONNECT_HELPER, ERROR_DELETE, ERROR_UNINSTALL, ERROR_MOVE_CUSTOM_DIR };

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
