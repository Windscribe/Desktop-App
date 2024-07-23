#include "dns_resolver.h"

#include <assert.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

#include "../logger.h"

DnsResolver *DnsResolver::this_ = NULL;

DnsResolver::DnsResolver() : channel_(NULL)
{
    assert(this_ == NULL);
    this_ = this;
    aresLibraryInit_.init();
}

DnsResolver::~DnsResolver()
{
    cancelAll();
    this_ = NULL;
}

void DnsResolver::setResolveDomainsCallbackHandler(std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    resolveDomainsCallback_ = resolveDomainsCallback;
}

void DnsResolver::stop()
{
    cancelAll();
}

void DnsResolver::resolveDomains(const std::vector<std::string> &hostnames)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // cancel any lookups currently in channel
    if (!hostnamesInProgress_.empty()) {
        ares_cancel(channel_);
        ares_destroy(channel_);
        ares_queue_wait_empty(channel_, -1);
        this_->channel_ = NULL;
    } else {
        assert(channel_ == NULL);
    }
    hostnamesInProgress_.clear();
    hostinfoResults_.clear();

    struct ares_options options;
    int optmask = 0;
    memset(&options, 0, sizeof(options));
    optmask |= ARES_OPT_TRIES;
    options.tries = 3;
    optmask |= ARES_OPT_EVENT_THREAD;
    options.evsys = ARES_EVSYS_DEFAULT;
    optmask |= ARES_OPT_MAXTIMEOUTMS;
    options.maxtimeout = 5 * 1000;

    int status = ares_init_options(&channel_, &options, optmask);
    if (status != ARES_SUCCESS) {
        Logger::instance().out("ares_init_options failed: %s", ares_strerror(status));
        return;
    }

    for (auto &hostname : hostnames) {
        hostnamesInProgress_.insert(hostname);
    }

    // filter for unique hostnames and execute request
    for (auto &hostname : hostnames) {
        USER_ARG *userArg = new USER_ARG();
        userArg->hostname = hostname;
        struct ares_addrinfo_hints hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        ares_getaddrinfo(channel_, hostname.c_str(), NULL, &hints, aresLookupFinishedCallback, userArg);
    }
}

void DnsResolver::cancelAll()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (channel_) {
        ares_cancel(channel_);
        hostnamesInProgress_.clear();
        ares_destroy(channel_);
        ares_queue_wait_empty(channel_, -1);
    }
    channel_ = NULL;
}

void DnsResolver::aresLookupFinishedCallback(void * arg, int status, int /*timeouts*/, struct ares_addrinfo *result)
{
    if (this_ == nullptr) {
        assert(false);
        return;
    };

    std::lock_guard<std::mutex> lock(this_->mutex_);

    USER_ARG *userArg = static_cast<USER_ARG *>(arg);

    // cancel and fail cases
    if (status == ARES_ECANCELLED) {
        delete userArg;
        return;
    }

    HostInfo hostInfo;
    hostInfo.hostname = userArg->hostname;

    if (status != ARES_SUCCESS) {
        hostInfo.error = true;
    } else {
        // add ips
        for (struct ares_addrinfo_node *node = result->nodes; node != NULL; node = node->ai_next) {
            char addr_buf[46] = "??";
            ares_inet_ntop(node->ai_family, &((const struct sockaddr_in *)((void *)node->ai_addr))->sin_addr, addr_buf, sizeof(addr_buf));
            hostInfo.addresses.push_back(std::string(addr_buf));
        }
    }

    this_->hostinfoResults_[hostInfo.hostname] = hostInfo;
    this_->hostnamesInProgress_.erase(userArg->hostname);

    if (this_->hostnamesInProgress_.empty()) {
        this_->resolveDomainsCallback_(this_->hostinfoResults_);
    }

    delete userArg;
}

