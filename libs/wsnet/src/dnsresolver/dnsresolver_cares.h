#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <set>
#include <condition_variable>

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
    void setAddressFamily(int addressFamily) override;

    std::shared_ptr<WSNetCancelableCallback> lookup(const std::string &hostname, std::uint64_t userDataId, WSNetDnsResolverCallback callback) override;
    std::shared_ptr<WSNetDnsRequestResult> lookupBlocked(const std::string &hostname) override;

private:
    void run();
    static void caresCallback(void *arg, int status, int timeouts, struct ares_addrinfo *result);

    static constexpr int kTimeoutMs = 2000;  // default value in c-ares, let's leave it as it is
    static constexpr int kTries = 2; // the number of tries the resolver will try contacting each name server before giving up.

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
    int addressFamily_;
};

} // namespace wsnet
