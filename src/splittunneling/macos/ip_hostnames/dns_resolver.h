#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#define CARES_NO_DEPRECATED     // Someday remove this and replace the functions with the new c-ares interface
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

    // Issues per-hostname AF_INET and AF_INET6 lookups in parallel. The all-resolved gate fires
    // only after both family answers (or per-family errors) are in for every hostname, so the
    // merged HostInfo entry contains every address that resolved across both families.
    void resolveDomains(const std::vector<std::string> &hostnames);
    void cancelAll();
    void setResolveDomainsCallbackHandler(std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback);

private:
    std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback_;

    struct USER_ARG
    {
        std::string hostname;
        int family;     // AF_INET or AF_INET6 — disambiguates the per-family completion bookkeeping.
    };

    bool bStopCalled_;
    std::mutex mutex_;
    std::condition_variable waitCondition_;
    bool bNeedFinish_;

    AresLibraryInit aresLibraryInit_;
    ares_channel channel_;

    static DnsResolver *this_;

    std::map<std::string, HostInfo> hostinfoResults_;
    // Per-hostname-and-family pending set. We use a set of (hostname, family) pairs rather than a
    // multiset of hostnames so the per-family completion erase is unambiguous (multiset::erase(key)
    // would clear both pending family entries at once, fooling the all-resolved gate below).
    std::set<std::pair<std::string, int>> hostnamesInProgress_;

    // thread specific
    std::thread thread_;
    static void threadFunc(void *arg);
    static void aresLookupFinishedCallback(void *arg, int status, int timeouts, const struct hostent *host);

    bool processChannel(ares_channel channel);
};
