#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <set>
#include <condition_variable>
#include <BS_thread_pool.hpp>

#include "WSNetDnsResolver.h"
#include "areslibraryinit.h"
#include "dnsservers.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

// DnsResolver implementation based on the cares library
// Thread safe
class DnsResolver_cares : public WSNetDnsResolver
{

public:
    explicit DnsResolver_cares();
    virtual ~DnsResolver_cares();

    bool init();

    void setDnsServers(const std::vector<std::string> &dnsServers) override;
    std::shared_ptr<WSNetCancelableCallback> lookup(const std::string &hostname, std::uint64_t userDataId, WSNetDnsResolverCallback callback) override;
    std::shared_ptr<WSNetDnsRequestResult> lookupBlocked(const std::string &hostname) override;

private:
    void run();
    static void caresCallback(void *arg, int status, int timeouts, struct hostent *host);

    // 200 ms settled for faster switching to the next try (next server)
    // this does not mean that the current request will be limited to 200ms,
    // but after 200 ms, the next one will start parallel to the first one
    // (see discussion for details: https://lists.haxx.se/pipermail/c-ares/2022-January/000032.html
    static constexpr int kTimeoutMs = 200;
    static constexpr int kTries = 4; // default value in c-ares, let's leave it as it is

    struct QueueItem
    {
        std::uint64_t requestId;
        std::string hostname;
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        std::uint64_t userDataId;
        std::shared_ptr<CancelableCallback<WSNetDnsResolverCallback>> callback;
    };

    struct ArgToCaresCallback
    {
        DnsResolver_cares *this_;
        QueueItem qi;
    };

    class DnsRequestResult : public WSNetDnsRequestResult
    {
    public:
        std::vector<std::string> ips() override { return ips_; }
        std::uint32_t elapsedMs() override { return elapsedMs_; }
        bool isError() override { return isError_; }
        std::string errorString() override { return errorString_; }

        std::vector<std::string> ips_;
        unsigned int elapsedMs_;
        bool isError_;
        std::string errorString_;
    };
    AresLibraryInit aresLibraryInit_;
    std::thread thread_;
    std::atomic_bool finish_ = false;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<QueueItem> queue_;
    std::set<std::uint64_t> activeRequests_;
    DnsServers dnsServers_;
    std::uint64_t curRequestId_;
};

} // namespace wsnet
