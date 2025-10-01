#pragma once
#include <string>
#include <functional>
#include <map>
#include <thread>

namespace wsnet {

typedef std::function<void(int)> PingFinishedCallbackFunc;

// Implement ICMP-pings on Apple in a recommanded Apple way
// We need a separate thread for non-blocking execution
class PingManager_apple
{
public:
    PingManager_apple();
    virtual ~PingManager_apple();

    void ping(const std::string &hostname, PingFinishedCallbackFunc callback);
    void stop();

private:
    std::thread thread_;
    std::mutex mutex_;
    void* runLoop_;  // CFRunLoopRef
    std::condition_variable runLoopReady_;
    bool isRunLoopReady_ = false;

    struct ActivePing
    {
        void *pingHelper;
        PingFinishedCallbackFunc callback;
    };

    std::uint64_t curId_ = 0;
    std::map<std::uint64_t, ActivePing> activePings_;
    void run();
    void callback(int timeMs, std::uint64_t id);
};


} // namespace wsnet
