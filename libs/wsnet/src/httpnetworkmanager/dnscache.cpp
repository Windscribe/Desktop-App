#include "dnscache.h"
#include <assert.h>
#include "utils/wsnet_logger.h"
#include "settings.h"
#include "utils/utils.h"
#include "utils/requesterror.h"

namespace wsnet {

DnsCache::DnsCache(WSNetDnsResolver *dnsResolver, DnsCacheCallback callback) :
    dnsResolver_(dnsResolver), callback_(callback)
{
}

DnsCache::~DnsCache()
{
    std::lock_guard locker(mutex_);
    for (auto &it : activeRequests_) {
        it.second->cancel();
    }
}

DnsCacheResult DnsCache::resolve(std::uint64_t id, const std::string &hostname, bool bypassCache)
{
    std::lock_guard locker(mutex_);
    assert(activeRequests_.find(id) == activeRequests_.end());

    if (!bypassCache) {
        auto it = cache_.find(hostname);
        if (it != cache_.end()) {
            return DnsCacheResult { id, true, it->second, 0, RequestError::createCaresSuccess() };
        }
    }
    auto asyncRequest = dnsResolver_->lookup(hostname, id, std::bind(&DnsCache::onDnsResolved, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    activeRequests_[id] = asyncRequest;
    return DnsCacheResult { id, false, std::vector<std::string>(), 0, RequestError::createCaresSuccess() };
}

void DnsCache::clear()
{
    std::lock_guard locker(mutex_);
    cache_.clear();
    g_logger->info("Clear DNS cache");
}

void DnsCache::onDnsResolved(std::uint64_t id, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
{
    std::lock_guard locker(mutex_);
    auto it = activeRequests_.find(id);
    assert(it != activeRequests_.end());

    // log no more than once per 1 second
    bool isNeedLog = true;
    if (tunnelTestLastLogTime_.has_value()) {
        auto timeSinceLastLog = utils::since(tunnelTestLastLogTime_.value()).count();
        isNeedLog = timeSinceLastLog >= 1000;
    }
    // useful log for tunnel test
    auto tunnelTestEndpoints = Settings::instance().tunnelTestEndpoints();
    if (isNeedLog && std::find(tunnelTestEndpoints.begin(), tunnelTestEndpoints.end(), hostname) != tunnelTestEndpoints.end()) {
        g_logger->info("DNS resolution for tunnel test ({}), result: {}, timems: {}", hostname, result->error()->toString(), result->elapsedMs());
        tunnelTestLastLogTime_ = std::chrono::steady_clock::now();
    }

    if (result->error()->isSuccess()) {
        cache_[hostname] = result->ips();
    } else {
        cache_.erase(hostname);
    }
    callback_(DnsCacheResult { id, false, result->ips(), result->elapsedMs(), result->error() } );

    activeRequests_.erase(it);
}

} // namespace wsnet

