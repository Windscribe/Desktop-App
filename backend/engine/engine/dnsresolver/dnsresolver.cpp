#include "dnsresolver.h"
#include "dnsutils.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"

#ifdef Q_OS_MAC
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

DnsResolver *DnsResolver::this_ = NULL;

DnsResolver::DnsResolver() : bNeedFinish_(false)
{
    Q_ASSERT(this_ == NULL);
    this_ = this;
    aresLibraryInit_.init();

    thread_ = std::thread(&DnsResolver::threadProc, this);
}

DnsResolver::~DnsResolver()
{
    qCDebug(LOG_BASIC) << "Stopping DnsResolver";
    {
        std::lock_guard<std::mutex> guard(mutex_);
        bNeedFinish_ = true;
        waitCondition_.notify_all();
   }
   try
   {
        thread_.join();
   }
   catch (...) { /* suppress */ };

   this_ = NULL;
   qCDebug(LOG_BASIC) << "DnsResolver stopped";
}

void DnsResolver::setDnsServers(const QStringList &ips)
{
    if (ips.empty())
    {
        qCDebug(LOG_BASIC) << "Changed DNS servers for DnsResolver to OS default";
    }
    else
    {
        qCDebug(LOG_BASIC) << "Changed DNS servers for DnsResolver to:" << ips;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    dnsServers_ = ips;
}

void DnsResolver::lookup(const std::string &hostname, const DnsResolverCallback &callback)
{
    std::shared_ptr<REQUEST_DATA> requestData(new REQUEST_DATA());

    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel(getDnsIps(), options, optmask, requestData.get());
    int status = ares_init_options(&requestData->channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        callback(std::vector<std::string>());
        return;
    }
    else
    {
        requestData->callback = callback;
        requestData->hostname = hostname;
        ares_gethostbyname(requestData->channel, hostname.c_str(), AF_INET, callbackForNonBlocked, requestData.get());
        {
            std::lock_guard<std::mutex> guard(mutex_);
            activeRequests_.push_back(requestData);
            waitCondition_.notify_all();
        }
    }
}


std::vector<std::string> DnsResolver::lookupBlocked(const std::string &hostname)
{
    /*DnsRequest *dnsRequest = new DnsRequest(this, hostname);
    ares_channel channel;

    struct ares_options options;
    int optmask = 0;

    QScopedPointer<ALLOCATED_DATA_FOR_OPTIONS> allocatedData (new ALLOCATED_DATA_FOR_OPTIONS());
    createOptionsForAresChannel(getDnsIps(), options, optmask, allocatedData.data());

    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        return dnsRequest;
    }

    USER_ARG_FOR_BLOCKED userArg;
    ares_gethostbyname(channel, hostname.toStdString().c_str(), AF_INET, callbackForBlocked, &userArg);

    // process loop
    timeval tv;
    while (1)
    {
        fd_set readers, writers;
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        int nfds = ares_fds(channel, &readers, &writers);
        if (nfds == 0)
        {
            break;
        }
        timeval *tvp = ares_timeout(channel, NULL, &tv);
        select(nfds, &readers, &writers, NULL, tvp);
        ares_process(channel, &readers, &writers);
    }

    ares_destroy(channel);

    dnsRequest->setIps(userArg.ips);
    return dnsRequest;*/
    return std::vector<std::string>();
}

void DnsResolver::detachCallbackAndRemoveRequests(const DnsResolverCallback &callback)
{

}

void DnsResolver::threadProc()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    while (true)
    {
        std::unique_lock<std::mutex> guard(mutex_);
        if (bNeedFinish_)
        {
            break;
        }

        bool bExistJob = false;

        auto it = activeRequests_.begin();
        while (it != activeRequests_.end())
        {
            if (processChannel((*it)->channel))
            {
                bExistJob = true;
                ++it;
            }
            else
            {
                ares_destroy((*it)->channel);
                it = activeRequests_.erase(it);
            }
        }

        if (!bExistJob)
        {
            // wait for new requests
            waitCondition_.wait(guard);
        }
    }

    // remove outstanding channels
    {
        std::lock_guard<std::mutex> guard(mutex_);
        for (auto it : activeRequests_)
        {
            ares_destroy(it->channel);
        }
        activeRequests_.clear();
    }
}


QStringList DnsResolver::getDnsIps()
{
    if (dnsServers_.empty())
    {
#if defined(Q_OS_MAC)
        QStringList osDefaultList;  // Empty by default.
        // On Mac, don't rely on automatic OS default DNS fetch in CARES, because it reads them from
        // the "/etc/resolv.conf", which is sometimes not available immediately after reboot.
        // Feed the CARES with valid OS default DNS values taken from scutil.
        const auto listDns = DnsUtils::getOSDefaultDnsServers();
        for (auto it = listDns.cbegin(); it != listDns.cend(); ++it)
            osDefaultList.push_back(QString::fromStdWString(*it));
        return osDefaultList;
#endif
    }
    return dnsServers_;
}

void DnsResolver::createOptionsForAresChannel(const QStringList &dnsIps, ares_options &options, int &optmask, REQUEST_DATA *requestData)
{
    memset(&options, 0, sizeof(options));
    optmask = 0;

    if (dnsIps.empty())
    {
        optmask |= ARES_OPT_TRIES;
        options.tries = 1;
    }
    else
    {
        optmask |= ARES_OPT_SERVERS;
        optmask |= ARES_OPT_TRIES;
        options.tries = 1;

        struct sockaddr_in sa;

        requestData->dnsServers.clear();
        requestData->dnsServers.reserve(dnsIps.size());
        for (const auto &dnsIp : dnsIps)
        {
            ares_inet_pton(AF_INET, dnsIp.toStdString().c_str(), &(sa.sin_addr));
            requestData->dnsServers.push_back(sa.sin_addr);
        }

        if (requestData->dnsServers.count() > 0)
        {
            options.nservers = requestData->dnsServers.count();
            options.servers = &(requestData->dnsServers[0]);
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}

void DnsResolver::callbackForNonBlocked(void *arg, int status, int timeouts, hostent *host)
{
    Q_UNUSED(timeouts);
    REQUEST_DATA *requestData = static_cast<REQUEST_DATA *>(arg);
    if (status != ARES_SUCCESS)
    {
        requestData->callback(std::vector<std::string>());
        return;
    }

    std::vector<std::string> addresses;
    for (char **p = host->h_addr_list; *p; p++)
    {
        char addr_buf[46] = "??";

        ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
        addresses.push_back(addr_buf);
    }

    requestData->callback(addresses);
    //userArg->dnsRequest->setIps(addresses);
    //emit userArg->dnsRequest->finished(userArg->dnsRequest);
    //delete userArg;
}

void DnsResolver::callbackForBlocked(void *arg, int status, int timeouts, hostent *host)
{
    Q_UNUSED(timeouts);
    USER_ARG_FOR_BLOCKED *userArg = static_cast<USER_ARG_FOR_BLOCKED *>(arg);

    if(status == ARES_SUCCESS)
    {
        QStringList addresses;
        for (char **p = host->h_addr_list; *p; p++)
        {
            char addr_buf[46] = "??";

            ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
            addresses << QString::fromStdString(addr_buf);
        }

        userArg->ips = addresses;
    }
    else
    {
        userArg->ips.clear();
    }
}

bool DnsResolver::processChannel(ares_channel channel)
{
    fd_set read_fds, write_fds;
    int res;
    int nfds;
    struct timeval tvp;
    tvp.tv_sec = 0;
    tvp.tv_usec = 10000; // 10 ms max waiting time

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    nfds = ares_fds(channel, &read_fds, &write_fds);
    if (nfds == 0)
    {
        return false;
    }

    res = select(nfds, &read_fds, &write_fds, NULL, &tvp);
    if (res == -1)
    {
        return false;
    }
    ares_process(channel, &read_fds, &write_fds);
    return true;
}
