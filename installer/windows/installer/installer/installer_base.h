#ifndef INSTALLER_BASE_H
#define INSTALLER_BASE_H

#ifdef _WIN32
#include <windows.h>
#endif

#if defined __APPLE__
#define E_ABORT   0x80004004L
#define S_OK      0L
#include <unistd.h>
#endif

#include <functional>    // std::function
#include <thread>        // std::thread
#include <mutex>         // std::mutex

enum INSTALLER_CURRENT_STATE {
    STATE_INIT,
    STATE_EXTRACTING,
    STATE_PAUSED,
    STATE_CANCELED,
    STATE_FINISHED,
    STATE_ERROR,
    STATE_FATAL_ERROR,
};

class InstallerBase
{
protected:
    std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> callbackState_;

    std::thread extract_thread;
    std::mutex mutex_;

    bool isPaused_;
    bool isCanceled_;
  
    std::wstring strLastError_;

    virtual void startImpl() = 0;
    virtual void executionImpl() = 0;
    virtual void runLauncherImpl() = 0;

private:
#ifdef GUI
    unsigned g_BreakCounter;
    unsigned kBreakAbortThreshold;
#else
#if defined __APPLE__
    void (*memo_sig_int)(int);
    void (*memo_sig_term)(int);
#endif
#endif

public:
#ifndef GUI
    static unsigned g_BreakCounter;
    static unsigned kBreakAbortThreshold;
#endif

    void start();
    void pause();
    void cancel();
    void resume();
    void waitForCompletion();
    void runLauncher();

    std::wstring getLastError() { return strLastError_; }

    InstallerBase(const std::function<void(unsigned int, INSTALLER_CURRENT_STATE)> &callbackState);
    virtual ~InstallerBase();
};

#endif // INSTALLER_BASE_H
