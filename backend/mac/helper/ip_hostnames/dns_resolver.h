#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ares_library_init.h"
#include "ares.h"

class DnsResolver
{
public:
    struct HostInfo
    {
        std::string hostname;
        std::vector<std::string> addresses;
        bool error;

        HostInfo() : error(false) {}
    };

    explicit DnsResolver();
    ~DnsResolver();
    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    void stop();

    void resolveDomains(const std::vector<std::string> &hostnames);
    void cancelAll();
    void setResolveDomainsCallbackHandler(std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback);

private:
    std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback_;

    struct USER_ARG {
        std::string hostname;
    };

    AresLibraryInit aresLibraryInit_;
    ares_channel channel_;
    std::mutex mutex_;

    static DnsResolver *this_;

    std::map<std::string, HostInfo> hostinfoResults_;
    std::set<std::string> hostnamesInProgress_;

    static void aresLookupFinishedCallback(void *arg, int status, int timeouts, struct ares_addrinfo *result);
};
