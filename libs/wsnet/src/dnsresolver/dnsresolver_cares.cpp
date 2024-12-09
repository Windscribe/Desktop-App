#define CARES_NO_DEPRECATED     // Someday remove this and replace the functions with the new c-ares interface
#include <ares.h>
#include "dnsresolver_cares.h"
#include <assert.h>
#include "utils/wsnet_logger.h"
#include "utils/utils.h"

#if defined(__APPLE__) || defined(__linux__)
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

namespace wsnet {

DnsResolver_cares::DnsResolver_cares() : curRequestId_(0), addressFamily_(AF_INET)
{
}

DnsResolver_cares::~DnsResolver_cares()
{
    finish_ = true;
    condition_.notify_all();
    thread_.join();
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
    dnsServers_ = DnsServers(dnsServers);
}

void DnsResolver_cares::setAddressFamily(int addressFamily)
{
    addressFamily_ = addressFamily;
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

            // get the current system DNS-server through a temporary channel
            ares_channel tempChannel;
            struct ares_options options;
            memset(&options, 0, sizeof(options));
            status = ares_init_options(&tempChannel, &options, 0);
            assert(status == ARES_SUCCESS);

            char *servers = ares_get_servers_csv(tempChannel);
            DnsServers dnsServersInTempChannel(servers);
            if (servers) {
                ares_free_string(servers);
            }

            if (dnsServersInChannel != dnsServersInTempChannel) {
                ares_cancel(channel);
                status = ares_set_servers_csv(channel, dnsServersInTempChannel.getAsCsv().c_str());
                assert(status == ARES_SUCCESS);
                dnsServersInChannel = dnsServersInTempChannel;

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

            struct ares_addrinfo_hints hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = addressFamily_;

            ares_getaddrinfo(channel, arg->qi.hostname.c_str(), NULL, &hints, caresCallback, arg);
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

void DnsResolver_cares::caresCallback(void *arg, int status, int timeouts, struct ares_addrinfo *results)
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
        for (struct ares_addrinfo_node *node = results->nodes; node != NULL; node = node->ai_next) {
            char addr_buf[46] = "??";
            if (node->ai_family == AF_INET) {
                ares_inet_ntop(node->ai_family, &((const struct sockaddr_in *)node->ai_addr)->sin_addr, addr_buf, sizeof(addr_buf));
            } else if (node->ai_family == AF_INET6) {
                ares_inet_ntop(node->ai_family, &((const struct sockaddr_in6 *)node->ai_addr)->sin6_addr, addr_buf, sizeof(addr_buf));
            } else {
                continue;
            }
            result->ips_.push_back(addr_buf);
        }
        result->isError_ = false;
    } else {
        result->errorString_ = ares_strerror(status);
        result->isError_ = true;
    }

    result->elapsedMs_ = (unsigned int)utils::since(pars->qi.startTime).count();

    // if the channel was destroyed, then do not call a callback function
    if (status != ARES_EDESTRUCTION) {
        // do callback
        pars->qi.callback->call(pars->qi.userDataId, pars->qi.hostname, result);
    }
    delete pars;
}

} // namespace wsnet
