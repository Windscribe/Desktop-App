#include "dnscache.h"
#include <assert.h>

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
            return DnsCacheResult { id, true, it->second, true };
        }
    }
    auto asyncRequest = dnsResolver_->lookup(hostname, id, std::bind(&DnsCache::onDnsResolved, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    activeRequests_[id] = asyncRequest;
    return DnsCacheResult { id, false, std::vector<std::string>(), false };
}

void DnsCache::onDnsResolved(std::uint64_t id, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
{
    std::lock_guard locker(mutex_);
    auto it = activeRequests_.find(id);
    assert(it != activeRequests_.end());

    if (!result->isError()) {
        cache_[hostname] = result->ips();
        callback_(DnsCacheResult { id, true, result->ips(), false } );

    } else {
        cache_.erase(hostname);
        callback_(DnsCacheResult { id, false, std::vector<std::string>(), false } );
    }

    activeRequests_.erase(it);
}

} // namespace wsnet

