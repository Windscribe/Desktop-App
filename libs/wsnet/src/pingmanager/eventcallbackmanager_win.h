#pragma once

#include <windows.h>
#include <functional>
#include <thread>
#include <mutex>
#include <map>

namespace wsnet {

typedef std::function<void(void)> EventCallbackFunc;

// Helper class for Windows icmp pings
// When an event is triggered it calls the specified callback
// thread safe
class EventCallbackManager_win
{
public:
    EventCallbackManager_win();
    virtual ~EventCallbackManager_win();
    void stop();
    void add(HANDLE hEvent, EventCallbackFunc callback);

private:
    std::thread thread_;
    std::mutex mutex_;
    HANDLE hEvent_;
    //std::condition_variable condition_;
    std::atomic_bool finish_ = false;
    bool isStopped_ = false;

    std::vector< std::pair<HANDLE, EventCallbackFunc> > events_;

    void run();
};

} // namespace wsnet
