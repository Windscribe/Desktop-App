#pragma once

#include <string>
#include <map>
#include <BS_thread_pool.hpp>
#include <wsnet/WSNet.h>

class DnsResolver
{
public:

    struct HostInfo
    {
        std::string hostname;
        std::vector<std::string> addresses;
        bool error = false;
    };

    explicit DnsResolver(std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback);
    ~DnsResolver();
    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    void resolveDomains(const std::vector<std::string> &hostnames);
    void cancelAll();

private:
    std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback_;
    BS::thread_pool taskQueue_;

    uint64_t curRequestId_ = 0;
    std::map<uint64_t, std::shared_ptr<wsnet::WSNetCancelableCallback>> activeRequests_;
    std::map<std::string, HostInfo> results_;

    void onDnsResolved(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result);
};

