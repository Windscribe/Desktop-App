#include "dns_resolver.h"

#include <assert.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

DnsResolver *DnsResolver::this_ = NULL;

DnsResolver::DnsResolver()
    : bStopCalled_(false)
    , bNeedFinish_(false)
    , channel_(NULL)
{
    assert(this_ == NULL);
    this_ = this;
    aresLibraryInit_.init();

    thread_ = std::thread(threadFunc, this);
}

DnsResolver::~DnsResolver()
{
    assert(bStopCalled_);
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

    // kick thread
    this_->mutex_.lock();
    bNeedFinish_ = true;
    waitCondition_.notify_all();
    this_->mutex_.unlock();

    // wait for thread to finish
    thread_.join();
    bStopCalled_ = true;
}

void DnsResolver::resolveDomains(const std::vector<std::string> &hostnames)
{
    std::lock_guard<std::mutex> lock(mutex_);

    {
        // cancel any lookups currently in channel
        if (!hostnamesInProgress_.empty())
        {
            ares_cancel(channel_);
            ares_destroy(channel_);
            this_->channel_ = NULL;
        }
        else
        {
            assert(channel_ == NULL);
        }
        hostnamesInProgress_.clear();
        hostinfoResults_.clear();

        struct ares_options options;
        int optmask = 0;
        memset(&options, 0, sizeof(options));
        optmask |= ARES_OPT_TRIES;
        options.tries = 3;

        int status = ares_init_options(&channel_, &options, optmask);
        if (status != ARES_SUCCESS)
        {
            spdlog::error("ares_init_options failed: {}", ares_strerror(status));
            return;
        }

        // Pre-populate the in-progress set with one (hostname, family) pair per request we'll
        // issue below. We deliberately do this in a separate loop so the all-resolved gate in
        // aresLookupFinishedCallback is only true once every request has had a chance to land:
        // racing pre-population with dispatch could let an early v4 answer fire the gate before
        // the v6 entry was inserted.
        for (auto &hostname : hostnames)
        {
            hostnamesInProgress_.insert({hostname, AF_INET});
            hostnamesInProgress_.insert({hostname, AF_INET6});
        }

        // Discrete v4 + v6 calls (rather than a single AF_UNSPEC call) keep per-family failure
        // modes visible to the callback — e.g. a v6-disabled host returning ARES_ENODATA on the
        // v6 leg shouldn't suppress the v4 answer, and vice versa.
        for (auto &hostname : hostnames)
        {
            USER_ARG *userArgV4 = new USER_ARG();
            userArgV4->hostname = hostname;
            userArgV4->family = AF_INET;
            ares_gethostbyname(channel_, hostname.c_str(), AF_INET, aresLookupFinishedCallback, userArgV4);

            USER_ARG *userArgV6 = new USER_ARG();
            userArgV6->hostname = hostname;
            userArgV6->family = AF_INET6;
            ares_gethostbyname(channel_, hostname.c_str(), AF_INET6, aresLookupFinishedCallback, userArgV6);
        }
    }

    // kick thread
    waitCondition_.notify_all();
}

void DnsResolver::cancelAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channel_)
    {
        ares_cancel(channel_);
        hostnamesInProgress_.clear();
        ares_destroy(channel_);
    }
    channel_ = NULL;
}


void DnsResolver::aresLookupFinishedCallback(void * arg, int status, int /*timeouts*/, const struct hostent * host)
{
    USER_ARG *userArg = static_cast<USER_ARG *>(arg);

    // cancel and fail cases
    if (status == ARES_ECANCELLED)
    {
        delete userArg;
        return;
    }

    // Merge into the existing per-hostname HostInfo entry so the v4 leg's addresses survive when
    // the v6 leg lands (and vice versa). We treat the merged entry as non-error if either family
    // succeeded — `error == true` only when *both* families failed.
    auto it = this_->hostinfoResults_.find(userArg->hostname);
    if (it == this_->hostinfoResults_.end())
    {
        HostInfo seed;
        seed.hostname = userArg->hostname;
        seed.error = true;     // flipped to false below if any family resolves successfully.
        it = this_->hostinfoResults_.emplace(userArg->hostname, std::move(seed)).first;
    }
    HostInfo &hostInfo = it->second;

    if (status == ARES_SUCCESS)
    {
        hostInfo.error = false;
        // add ips
        for (char **p = host->h_addr_list; *p; p++)
        {
            char addr_buf[46] = "??";
            ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
            hostInfo.addresses.push_back(std::string(addr_buf));
        }
    }
    // If status != ARES_SUCCESS we leave hostInfo.error as-is: a prior successful family clears it,
    // and a not-yet-completed-but-pending family will get its own chance to flip it.

    this_->hostnamesInProgress_.erase({userArg->hostname, userArg->family});

    if (this_->hostnamesInProgress_.empty())
    {
        this_->resolveDomainsCallback_(this_->hostinfoResults_);
    }

    delete userArg;
}

void DnsResolver::threadFunc(void *arg)
{
    //BIND_CRASH_HANDLER_FOR_THREAD();
    DnsResolver *resolver = static_cast<DnsResolver *>(arg);

    while (true)
    {
        {
            std::unique_lock<std::mutex> lockWait(resolver->mutex_);
            if (resolver->bNeedFinish_)
            {
                break;
            }

            if (resolver->channel_ != NULL && !resolver->processChannel(resolver->channel_))
            {
                resolver->waitCondition_.wait(lockWait);
            }
        }
        sleep(1); // NOLINT: false positive, sleep is outside the critical section
    }
}

bool DnsResolver::processChannel(ares_channel channel)
{
    fd_set read_fds, write_fds;
    int res;
    int nfds;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    nfds = ares_fds(channel, &read_fds, &write_fds);
    if (nfds == 0) {
        return false;
    }
    timeval tv;
    timeval *tvp = ares_timeout(channel, NULL, &tv);
    (void)select(nfds, &read_fds, &write_fds, NULL, tvp);
    ares_process(channel, &read_fds, &write_fds);
    return true;
}
