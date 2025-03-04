#pragma once
#include <string>
#include <map>
#include <mutex>
#include <optional>
#include "WSNetDnsResolver.h"

namespace wsnet {

struct DnsCacheResult
{
    std::uint64_t id;
    bool bFromCache;
    std::vector<std::string> ips;
    std::uint32_t elapsedMs;
    std::shared_ptr<WSNetRequestError> error;
};

typedef std::function<void(const DnsCacheResult &result)> DnsCacheCallback;

// TODO: add TTL support, remove from cache + whitelist ips handler
class DnsCache final
{
public:
    explicit DnsCache(WSNetDnsResolver *dnsResolver, DnsCacheCallback callback);
    ~DnsCache();

    DnsCacheResult resolve(std::uint64_t id, const std::string &hostname, bool bypassCache = false);
    void clear();

private:
    WSNetDnsResolver *dnsResolver_;
    DnsCacheCallback callback_;
    std::mutex mutex_;
    std::map<std::string, std::vector<std::string> > cache_;
    std::map<std::uint64_t, std::shared_ptr<WSNetCancelableCallback> > activeRequests_;

    std::optional<std::chrono::time_point<std::chrono::steady_clock> > tunnelTestLastLogTime_;

    void onDnsResolved(std::uint64_t id, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result);
};

} // namespace wsnet
