#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "scapix_object.h"

#include "WSNetDnsRequestResult.h"
#include "WSNetCancelableCallback.h"

namespace wsnet {

typedef std::function<void(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)> WSNetDnsResolverCallback;

// Async thread safe DNS resolver, you can call class functions from any thread.
class WSNetDnsResolver : public scapix_object<WSNetDnsResolver>
{
public:
    virtual ~WSNetDnsResolver() {}

    virtual void setDnsServers(const std::vector<std::string> &dnsServers) = 0;

    virtual std::shared_ptr<WSNetCancelableCallback> lookup(const std::string &hostname, std::uint64_t requestId, WSNetDnsResolverCallback callback) = 0;
    virtual std::shared_ptr<WSNetDnsRequestResult> lookupBlocked(const std::string &hostname) = 0;
};

} // namespace wsnet

