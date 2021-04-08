#include "dnsresolver.h"
#include "dnsutils.h"
#include "engine/hardcodedsettings.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"

#include <QElapsedTimer>

#ifdef Q_OS_MAC
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

DnsResolver *DnsResolver::this_ = NULL;
const int typeIdQHostInfo = qRegisterMetaType<QHostInfo>("QHostInfo");

void DnsResolver::runTests()
{
    //todo -> move to test project
    test_ = new DnsResolver_test(this);
    test_->runTests();
}

void DnsResolver::stop()
{
    qCDebug(LOG_BASIC) << "Stopping DnsResolver";
    test_->stop();
    mutex_.lock();
    bNeedFinish_ = true;
    waitCondition_.wakeAll();
    mutex_.unlock();
    wait();
    bStopCalled_ = true;
    qCDebug(LOG_BASIC) << "DnsResolver stopped";
}

void DnsResolver::setDnsPolicy(DNS_POLICY_TYPE dnsPolicyType)
{
    QMutexLocker locker(&mutex_);
    dnsPolicyType_ = dnsPolicyType;
}

void DnsResolver::setUseCustomDns(bool bUseCustomDns)
{
    isUseCustomDns_ = bUseCustomDns;

    if (bUseCustomDns)
    {
        qCDebug(LOG_BASIC) << "Changed DNS mode to custom";
    }
    else
    {
        qCDebug(LOG_BASIC) << "Changed DNS mode to automatic";
    }
}

QHostInfo DnsResolver::lookupBlocked(const QString &hostname)
{
    ares_channel channel;

    struct ares_options options;
    int optmask = 0;

    QScopedPointer<ALLOCATED_DATA_FOR_OPTIONS> allocatedData (new ALLOCATED_DATA_FOR_OPTIONS());
    createOptionsForAresChannel((!isUseCustomDns_) ? QStringList() : getCustomDnsIps(), options, optmask, allocatedData.data());

    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        return QHostInfo();
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
    return userArg.ha;
}


DnsResolver::DnsResolver(QObject *parent) : QThread(parent), bStopCalled_(false),
    bNeedFinish_(false), dnsPolicyType_(DNS_TYPE_OPEN_DNS),
    isUseCustomDns_(true)
{
    Q_ASSERT(this_ == NULL);
    this_ = this;
    aresLibraryInit_.init();

    start(QThread::LowPriority);
}

DnsResolver::~DnsResolver()
{
    Q_ASSERT(bStopCalled_);
    this_ = NULL;
}

QStringList DnsResolver::getCustomDnsIps()
{
    if (dnsPolicyType_ == DNS_TYPE_OS_DEFAULT)
    {
        QStringList osDefaultList;  // Empty by default.
#if defined(Q_OS_MAC)
        // On Mac, don't rely on automatic OS default DNS fetch in CARES, because it reads them from
        // the "/etc/resolv.conf", which is sometimes not available immediately after reboot.
        // Feed the CARES with valid OS default DNS values taken from scutil.
        const auto listDns = DnsUtils::getDnsServers();
        for (auto it = listDns.cbegin(); it != listDns.cend(); ++it)
            osDefaultList.push_back(QString::fromStdWString(*it));
#endif
        return osDefaultList;
    }
    else if (dnsPolicyType_ == DNS_TYPE_OPEN_DNS)
    {
        return HardcodedSettings::instance().customDns();
    }
    else if (dnsPolicyType_ == DNS_TYPE_CLOUDFLARE)
    {
        return QStringList() << HardcodedSettings::instance().cloudflareDns();
    }
    else if (dnsPolicyType_ == DNS_TYPE_GOOGLE)
    {
        return QStringList() << HardcodedSettings::instance().googleDns();
    }
    else
    {
        Q_ASSERT(false);
    }
    return QStringList();
}

void DnsResolver::createOptionsForAresChannel(const QStringList &dnsIps, ares_options &options, int &optmask, ALLOCATED_DATA_FOR_OPTIONS *allocatedData)
{
    memset(&options, 0, sizeof(options));
    optmask = 0;

    if (dnsIps.isEmpty())
    {
        optmask |= ARES_OPT_DOMAINS;
        optmask |= ARES_OPT_TRIES;
        options.ndomains = 1;
        options.domains = &(allocatedData->domainPtr);
        options.tries = 1;
    }
    else
    {
        optmask |= ARES_OPT_DOMAINS;
        optmask |= ARES_OPT_SERVERS;
        optmask |= ARES_OPT_TRIES;
        options.ndomains = 1;
        options.domains = &(allocatedData->domainPtr);
        options.tries = 1;

        struct sockaddr_in sa;

        allocatedData->dnsServers.clear();
        allocatedData->dnsServers.reserve(dnsIps.count());
        for (const auto &dnsIp : dnsIps)
        {
            ares_inet_pton(AF_INET, dnsIp.toStdString().c_str(), &(sa.sin_addr));
            allocatedData->dnsServers.push_back(sa.sin_addr);
        }

        if (allocatedData->dnsServers.count() > 0)
        {
            options.nservers = allocatedData->dnsServers.count();
            options.servers = &(allocatedData->dnsServers[0]);
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
        emit this_->resolved(userArg->hostname, QHostInfo(), userArg->userPointer);
        delete userArg;
        return;
    }

    QList<QHostAddress> addresses;
    for (char **p = host->h_addr_list; *p; p++)
    {
        char addr_buf[46] = "??";

        ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
        addresses << QHostAddress(QString::fromStdString(addr_buf));
    }

    QHostInfo hostInfo;
    hostInfo.setAddresses(addresses);
    hostInfo.setError(QHostInfo::NoError);
    emit this_->resolved(userArg->hostname, hostInfo, userArg->userPointer);
    delete userArg;
}

void DnsResolver::callbackForBlocked(void *arg, int status, int timeouts, hostent *host)
{
    Q_UNUSED(timeouts);
    USER_ARG_FOR_BLOCKED *userArg = static_cast<USER_ARG_FOR_BLOCKED *>(arg);

    if(status == ARES_SUCCESS)
    {
        QList<QHostAddress> addresses;
        for (char **p = host->h_addr_list; *p; p++)
        {
            char addr_buf[46] = "??";

            ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
            addresses << QHostAddress(QString::fromStdString(addr_buf));
        }

        QHostInfo hostInfo;
        hostInfo.setAddresses(addresses);
        hostInfo.setError(QHostInfo::NoError);
        userArg->ha = hostInfo;
    }
    else
    {
        userArg->ha = QHostInfo();
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


void DnsResolver::lookup(const QString &hostname, void *userPointer)
{
    QElapsedTimer timer;
    timer.start();
    ares_channel channel;
    struct ares_options options;
    int optmask = 0;

    ALLOCATED_DATA_FOR_OPTIONS *allocatedData = new ALLOCATED_DATA_FOR_OPTIONS();

    createOptionsForAresChannel((!isUseCustomDns_) ? QStringList() : getCustomDnsIps(), options, optmask, allocatedData);
    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        delete allocatedData;
        emit resolved(hostname, QHostInfo(), userPointer);
    }
    else
    {
        USER_ARG *userArg = new USER_ARG();
        userArg->hostname = hostname;
        userArg->userPointer = userPointer;

        ares_gethostbyname(channel, hostname.toStdString().c_str(), AF_INET, callback, userArg);

        {
            QMutexLocker locker(&mutex_);
            CHANNEL_INFO ci;
            ci.channel = channel;
            ci.allocatedData = allocatedData;
            queue_.enqueue(ci);
            waitCondition_.wakeAll();
        }
    }
    qDebug() << "lookup:" << hostname << "; time = " << timer.elapsed() << ((!isUseCustomDns_) ? QStringList() : getCustomDnsIps());
}

void DnsResolver::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    QVector<CHANNEL_INFO> channels;

    while (true)
    {
        QMutexLocker locker(&mutex_);
        if (bNeedFinish_)
        {
            break;
        }

        while (!queue_.isEmpty())
        {
            channels << queue_.dequeue();
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
                delete it->allocatedData;
                it = channels.erase(it);
            }
        }

        if (!bExistJob)
        {
            // wait for new requests
            waitCondition_.wait(&mutex_);
        }
    }

    // remove outstanding channels
    {
        QMutexLocker locker(&mutex_);
        while (!queue_.isEmpty())
        {
            channels << queue_.dequeue();
        }
    }
    QVector<CHANNEL_INFO>::iterator it = channels.begin();
    while (it != channels.end())
    {
        ares_destroy(it->channel);
        delete it->allocatedData;
        it = channels.erase(it);
    }
}

DnsResolver::ALLOCATED_DATA_FOR_OPTIONS::ALLOCATED_DATA_FOR_OPTIONS()
{
    strcpy(szDomain, HardcodedSettings::instance().serverApiUrl().toStdString().c_str());
    domainPtr = szDomain;
}
