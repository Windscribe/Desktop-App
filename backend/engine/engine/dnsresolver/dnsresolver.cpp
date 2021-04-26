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

DnsResolver::DnsResolver(QObject *parent) : QThread(parent), bStopCalled_(false),
    bNeedFinish_(false)
{
    Q_ASSERT(this_ == NULL);
    this_ = this;
    aresLibraryInit_.init();

    start(QThread::LowPriority);
}

DnsResolver::~DnsResolver()
{
    qCDebug(LOG_BASIC) << "Stopping DnsResolver";
    mutex_.lock();
    bNeedFinish_ = true;
    waitCondition_.wakeAll();
    mutex_.unlock();
    wait();
    bStopCalled_ = true;
    qCDebug(LOG_BASIC) << "DnsResolver stopped";
    this_ = NULL;
}

void DnsResolver::lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers)
{
    QMutexLocker locker(&mutex_);
    REQUEST_INFO ri;
    ri.hostname = hostname;
    ri.object = object;
    ri.dnsServers = dnsServers;
    queue_.enqueue(ri);
    waitCondition_.wakeAll();
}

QStringList DnsResolver::lookupBlocked(const QString &hostname, const QStringList &dnsServers)
{
    struct ares_options options;
    int optmask = 0;

    QScopedPointer<CHANNEL_INFO> channelInfo (new CHANNEL_INFO());
    createOptionsForAresChannel(getDnsIps(dnsServers), options, optmask, channelInfo.data());

    int status = ares_init_options(&(channelInfo->channel), &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        return QStringList();
    }

    USER_ARG_FOR_BLOCKED userArg;
    ares_gethostbyname(channelInfo->channel, hostname.toStdString().c_str(), AF_INET, callbackForBlocked, &userArg);

    // process loop
    timeval tv;
    while (1)
    {
        fd_set readers, writers;
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        int nfds = ares_fds(channelInfo->channel, &readers, &writers);
        if (nfds == 0)
        {
            break;
        }
        timeval *tvp = ares_timeout(channelInfo->channel, NULL, &tv);
        select(nfds, &readers, &writers, NULL, tvp);
        ares_process(channelInfo->channel, &readers, &writers);
    }

    ares_destroy(channelInfo->channel);
    return userArg.ips;
}

void DnsResolver::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    QVector<CHANNEL_INFO> channels;

    while (true)
    {
        mutex_.lock();
        REQUEST_INFO ri;
        bool bExistRequest = false;
        if (!queue_.isEmpty())
        {
            ri = queue_.dequeue();
            bExistRequest = true;
        }
        mutex_.unlock();

        if (bExistRequest)
        {
            CHANNEL_INFO channelInfo;
            if (initChannel(ri, channelInfo))
            {
                channels << channelInfo;
            }
            else
            {
                bool bSuccess = QMetaObject::invokeMethod(ri.object.get(), "onResolved",
                                          Qt::QueuedConnection, Q_ARG(QStringList, QStringList()));
                Q_ASSERT(bSuccess);
            }
        }

        bool bExistJob = false;
        QVector<CHANNEL_INFO>::iterator it = channels.begin();
        while (it != channels.end())
        {
            if (processChannel(it->channel))
            {
                bExistJob = true;
                ++it;
            }
            else
            {
                ares_destroy(it->channel);
                it = channels.erase(it);
            }
        }

        {
            QMutexLocker locker(&mutex_);
            while (!bExistJob && queue_.isEmpty() && !bNeedFinish_)
            {
                waitCondition_.wait(&mutex_);
            }
            if (bNeedFinish_)
            {
                break;
            }
        }
    }

    // remove outstanding channels
    {
        QMutexLocker locker(&mutex_);
        queue_.clear();
    }
    QVector<CHANNEL_INFO>::iterator it = channels.begin();
    while (it != channels.end())
    {
        ares_destroy(it->channel);
        it = channels.erase(it);
    }
}


QStringList DnsResolver::getDnsIps(const QStringList &ips)
{
    if (ips.isEmpty())
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
    return ips;
}

void DnsResolver::createOptionsForAresChannel(const QStringList &dnsIps, ares_options &options, int &optmask, CHANNEL_INFO *channelInfo)
{
    memset(&options, 0, sizeof(options));
    optmask = 0;

    if (dnsIps.isEmpty())
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

        channelInfo->dnsServers.clear();
        channelInfo->dnsServers.reserve(dnsIps.count());
        for (const auto &dnsIp : dnsIps)
        {
            ares_inet_pton(AF_INET, dnsIp.toStdString().c_str(), &(sa.sin_addr));
            channelInfo->dnsServers.push_back(sa.sin_addr);
        }

        if (channelInfo->dnsServers.count() > 0)
        {
            options.nservers = channelInfo->dnsServers.count();
            options.servers = &(channelInfo->dnsServers[0]);
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}

void DnsResolver::callback(void *arg, int status, int timeouts, hostent *host)
{
    Q_UNUSED(timeouts);
    USER_ARG *userArg = static_cast<USER_ARG *>(arg);
    if (status != ARES_SUCCESS)
    {
        bool bSuccess = QMetaObject::invokeMethod(userArg->object.get(), "onResolved",
                                  Qt::QueuedConnection, Q_ARG(QStringList, QStringList()));
        Q_ASSERT(bSuccess);
        delete userArg;
        return;
    }

    QStringList addresses;
    for (char **p = host->h_addr_list; *p; p++)
    {
        char addr_buf[46] = "??";

        ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
        addresses << QString::fromStdString(addr_buf);
    }

    bool bSuccess = QMetaObject::invokeMethod(userArg->object.get(), "onResolved",
                              Qt::QueuedConnection, Q_ARG(QStringList, addresses));
    Q_ASSERT(bSuccess);

    delete userArg;
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

bool DnsResolver::initChannel(const REQUEST_INFO &ri, CHANNEL_INFO &outChannelInfo)
{
    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel(getDnsIps(ri.dnsServers), options, optmask, &outChannelInfo);
    int status = ares_init_options(&outChannelInfo.channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        return false;
    }
    else
    {
        USER_ARG *userArg = new USER_ARG();
        userArg->hostname = ri.hostname;
        userArg->object = ri.object;

        ares_gethostbyname(outChannelInfo.channel, ri.hostname.toStdString().c_str(), AF_INET, callback, userArg);
        return true;
    }
}
