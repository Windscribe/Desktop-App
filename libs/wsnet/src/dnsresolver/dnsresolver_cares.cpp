#define CARES_NO_DEPRECATED     // Someday remove this and replace the functions with the new c-ares interface
#include <ares.h>
#include "dnsresolver_cares.h"
#include <assert.h>
#include <fmt/ranges.h>
#include "utils/wsnet_logger.h"
#include "utils/requesterror.h"
#include "utils/utils.h"

#if defined(__APPLE__) || defined(__linux__)
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

#if defined(__APPLE__) && !defined(IS_MOBILE_PLATFORM)
    #include "getdnsconfig_mac.h"
#endif


namespace wsnet {

DnsResolver_cares::DnsResolver_cares() : curRequestId_(0)
{
}

DnsResolver_cares::~DnsResolver_cares()
{
    g_logger->info("DnsResolver_cares destructor started");
    finish_ = true;
    condition_.notify_all();
    thread_.join();
    g_logger->info("DnsResolver_cares destructor finished");
}

bool DnsResolver_cares::init()
{
    if (aresLibraryInit_.init()) {
        thread_ = std::thread(std::bind(&DnsResolver_cares::run, this));
        return true;
    }
    return false;
}

void DnsResolver_cares::setDnsServers(const std::vector<std::string> &dnsServers)
{
    std::lock_guard locker(mutex_);
    g_logger->info("Set dns servers by client: {}", fmt::join(dnsServers, ","));
    dnsServers_ = DnsServers(dnsServers);
}

std::shared_ptr<WSNetCancelableCallback> DnsResolver_cares::lookup(const std::string &hostname, std::uint64_t userDataId, WSNetDnsResolverCallback callback)
{
    std::lock_guard locker(mutex_);
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetDnsResolverCallback>>(callback);

    QueueItem qi;
    qi.hostname = hostname;
    qi.callback = cancelableCallback;
    qi.userDataId = userDataId;
    qi.requestId = curRequestId_++;

    activeRequests_.insert(qi.requestId);
    queue_.push(qi);
    condition_.notify_all();

    return cancelableCallback;
}

std::shared_ptr<WSNetDnsRequestResult> DnsResolver_cares::lookupBlocked(const std::string &hostname)
{
    // helper class for wait an async request
    class BlockedRequest {
        public:
            BlockedRequest(WSNetDnsResolver *dnsResolver, const std::string &hostname)
            {
                dnsResolver->lookup(hostname, 0, std::bind(&BlockedRequest::callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            }

            std::shared_ptr<WSNetDnsRequestResult> waitForFinished()
            {
                std::unique_lock locker(mutex_);
                cv_.wait(locker, [this] { return isFinished_; });
                return result_;
            }
        private:
            void callback(std::uint64_t id, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
            {
                std::lock_guard locker(mutex_);
                result_ = result;
                isFinished_ = true;
                cv_.notify_all();
            }
            std::mutex mutex_;
            std::condition_variable cv_;
            bool isFinished_ = false;
            std::shared_ptr<WSNetDnsRequestResult> result_;
    };

    BlockedRequest blockedRequest(this, hostname);
    return blockedRequest.waitForFinished();
}

void DnsResolver_cares::run()
{
    ares_channel channel;
    struct ares_options options;
    int optmask;

    memset(&options, 0, sizeof(options));
    optmask = ARES_OPT_TRIES | ARES_OPT_TIMEOUTMS | ARES_OPT_MAXTIMEOUTMS;
    options.tries = kTries;
    options.timeout = kTimeoutMs;
    options.maxtimeout = kTimeoutMs;

    [[maybe_unused]] int status = ares_init_options(&channel, &options, optmask);
    assert(status == ARES_SUCCESS);

    DnsServers dnsServersInstalled;
    char *servers = ares_get_servers_csv(channel);
    DnsServers dnsServersInChannel(servers);
    if (servers) {
        ares_free_string(servers);
    }

    g_logger->info("DNS servers in channel: {}", dnsServersInChannel.getAsCsv());

    std::queue<QueueItem> localQueue;
    while (!finish_) {

        {   // mutex lock section
            std::unique_lock<std::mutex> locker(mutex_);
            condition_.wait(locker, [this]{ return !activeRequests_.empty() || finish_; });
            localQueue = queue_;
            queue_ = std::queue<QueueItem>();
            if (dnsServersInstalled != dnsServers_) {
                dnsServersInstalled = dnsServers_;
            }
        }

        // Check if the list of DNS-servers has changed
        // We must to cancel current requests before installing new DNS-servers
        if (dnsServersInstalled.isEmpty()) {    // Use default system DNS-servers

            DnsServers dnsServersDefaultConfiguration;
            // get the current system DNS-server through a temporary channel
            ares_channel tempChannel;
            struct ares_options options;
            memset(&options, 0, sizeof(options));
            // ARES_FLAG_NO_DFLT_SVR flag => do not attempt to add a default local named server if there are no other servers available
            status = ares_init_options(&tempChannel, &options, ARES_FLAG_NO_DFLT_SVR);
            if (status != ARES_SUCCESS) {
                // Sometimes we see a bug on MacOS where c-ares cannot get the DNS configuration and uses the 127.0.0.1 address instead
                // To make c-ares return an error instead of using the 127.0.0.1 address, we use the flag ARES_FLAG_NO_DFLT_SVR in ares_init_options call above
                // It seems the method that c-ares uses to detect DNS configuration is not 100% reliable.
                // The implementation of the method is here: https://github.com/c-ares/c-ares/blob/main/src/lib/ares_sysconfig_mac.c
                // So if the c-ares method fails, let's try another method using the System Configuration Framework.
#if defined(__APPLE__) && !defined(IS_MOBILE_PLATFORM)
                auto strDnsServers = getDnsConfig_mac();
                logDnsServersFromSystemConfigurationFrameworkIfChanged(strDnsServers);
                dnsServersDefaultConfiguration = DnsServers(strDnsServers.c_str());
#endif
            } else {
                char *servers = ares_get_servers_csv(tempChannel);
                dnsServersDefaultConfiguration = DnsServers(servers);
                if (servers) {
                    ares_free_string(servers);
                }
            }

            if (dnsServersInChannel != dnsServersDefaultConfiguration) {
                ares_cancel(channel);
                status = ares_set_servers_csv(channel, dnsServersDefaultConfiguration.getAsCsv().c_str());
                assert(status == ARES_SUCCESS);
                dnsServersInChannel = dnsServersDefaultConfiguration;

                g_logger->info("DNS servers in channel are changed: {}", dnsServersInChannel.getAsCsv());
            }
            ares_destroy(tempChannel);

        } else {
            if (dnsServersInChannel != dnsServersInstalled) {
                ares_cancel(channel);
                status = ares_set_servers_csv(channel, dnsServersInstalled.getAsCsv().c_str());
                if (status != ARES_SUCCESS) {
                    g_logger->error("Failed to set DNS servers to channel: {}", dnsServersInstalled.getAsCsv());
                } else {
                    dnsServersInChannel = dnsServersInstalled;
                    g_logger->info("DNS servers in channel are changed: {}", dnsServersInChannel.getAsCsv());
                }
            }
        }

        // start new requests from the queue
        while (!localQueue.empty()) {
            QueueItem qi = localQueue.front();
            ArgToCaresCallback *arg = new ArgToCaresCallback();     // will be deleted in caresCallback
            arg->this_ = this;
            arg->qi = qi;
            arg->qi.startTime = std::chrono::steady_clock::now();

            ares_gethostbyname(channel, arg->qi.hostname.c_str(), AF_INET, caresCallback, arg);
            localQueue.pop();
        }

        // process
        fd_set readers, writers;
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        int nfds = ares_fds(channel, &readers, &writers);
        if (nfds != 0) {
            // do not block for longer than kTimeoutMs interval
            timeval tvp;
            tvp.tv_sec = 0;
            tvp.tv_usec = kTimeoutMs * 1000;
            select(nfds, &readers, &writers, NULL, &tvp);
            ares_process(channel, &readers, &writers);
        }
    }

    ares_destroy(channel);
}

void DnsResolver_cares::caresCallback(void *arg, int status, int timeouts, hostent *host)
{
    ArgToCaresCallback *pars = (ArgToCaresCallback *)arg;

    {
        std::lock_guard locker(pars->this_->mutex_);
        auto it = pars->this_->activeRequests_.find(pars->qi.requestId);
        if (it != pars->this_->activeRequests_.end()) {
            pars->this_->activeRequests_.erase(it);
        } else {
            assert(false);
        }
    }

    std::shared_ptr<DnsRequestResult> result = std::make_shared<DnsRequestResult>();
    if (status == ARES_SUCCESS) {
        for (char **p = host->h_addr_list; *p; p++) {
            char addr_buf[46] = "??";
            ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
            result->ips_.push_back(addr_buf);
        }
    }

    result->elapsedMs_ = (unsigned int)utils::since(pars->qi.startTime).count();
    result->error_ = std::make_shared<RequestError>(status, RequestErrorType::kCares);

    // if the channel was destroyed, then do not call a callback function
    if (status != ARES_EDESTRUCTION) {
        // do callback
        pars->qi.callback->call(pars->qi.userDataId, pars->qi.hostname, result);
    }
    delete pars;
}

void DnsResolver_cares::logDnsServersFromSystemConfigurationFrameworkIfChanged(const std::string &servers)
{
    // log only if changed to avoid spam
    static std::string lastServers;
    if (lastServers != servers) {
        lastServers = servers;
        g_logger->info("DNS servers from System Configuration Framework: {}", servers);
    }
}

} // namespace wsnet
