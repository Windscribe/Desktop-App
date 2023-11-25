#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
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

    struct USER_ARG
    {
        std::string hostname;
    };

    bool bStopCalled_;
    std::mutex mutex_;
    std::condition_variable waitCondition_;
    bool bNeedFinish_;

    AresLibraryInit aresLibraryInit_;
    ares_channel channel_;

    static DnsResolver *this_;

    std::map<std::string, HostInfo> hostinfoResults_;
    std::set<std::string> hostnamesInProgress_;

    // thread specific
    std::thread thread_;
    static void threadFunc(void *arg);
    static void aresLookupFinishedCallback(void *arg, int status, int timeouts, struct hostent *host);

    bool processChannel(ares_channel channel);
};

