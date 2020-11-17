#include "dnsresolver.h"
#include "engine/hardcodedsettings.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"

#ifdef Q_OS_MAC
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

DnsResolver *DnsResolver::this_ = NULL;

const int typeIdQHostInfo = qRegisterMetaType<QHostInfo>("QHostInfo");

void DnsResolver::init()
{

}

void DnsResolver::stop()
{
    mutexWait_.lock();
    bNeedFinish_ = true;
    waitCondition_.wakeAll();
    mutexWait_.unlock();
    wait();
    bStopCalled_ = true;
}

void DnsResolver::setDnsPolicy(DNS_POLICY_TYPE dnsPolicyType)
{
    if (dnsPolicyType_ != dnsPolicyType)
    {
        dnsPolicyType_ = dnsPolicyType;
        recreateCustomDnsChannel();
    }
}

void DnsResolver::recreateDefaultDnsChannel()
{
    mutex_.lock();
    if (channel_)
    {
        ares_destroy(channel_);
        channel_ = NULL;
    }

    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel(QStringList(), options, optmask);

    int status = ares_init_options(&channel_, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        channel_ = NULL;
    }

    if (dnsPolicyType_ == DNS_TYPE_OS_DEFAULT)
    {
        if (channelCustomDns_)
        {
            ares_destroy(channelCustomDns_);
            channelCustomDns_ = NULL;
        }

        createOptionsForAresChannel(QStringList(), options, optmask);

        status = ares_init_options(&channelCustomDns_, &options, optmask);
        if (status != ARES_SUCCESS)
        {
            qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
            channelCustomDns_ = NULL;
        }
    }

    mutex_.unlock();
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

void DnsResolver::lookup(const QString &hostname, void *userPointer)
{
    mutex_.lock();

    if (channelCustomDns_ == NULL || channel_ == NULL)
    {
        qCDebug(LOG_BASIC) << "Error: ares channels not created before DNS-request";
        emit resolved(hostname, QHostInfo(), userPointer);
    }
    USER_ARG *userArg = new USER_ARG();
    userArg->hostname = hostname;
    userArg->userPointer = userPointer;

    if (isUseCustomDns_)
    {
        ares_gethostbyname(channelCustomDns_, hostname.toStdString().c_str(), AF_INET, callback, userArg);
    }
    else
    {
        ares_gethostbyname(channel_, hostname.toStdString().c_str(), AF_INET, callback, userArg);
    }
    mutex_.unlock();

    mutexWait_.lock();
    waitCondition_.wakeAll();
    mutexWait_.unlock();
}


QHostInfo DnsResolver::lookupBlocked(const QString &hostname)
{
    ares_channel channel;

    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel((!isUseCustomDns_) ? QStringList() : getCustomDnsIps(), options, optmask);
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
    bNeedFinish_(false), channel_(NULL), channelCustomDns_(NULL), dnsPolicyType_(DNS_TYPE_OPEN_DNS),
    isUseCustomDns_(true)
{
    Q_ASSERT(this_ == NULL);
    this_ = this;
    strcpy(szDomain_, HardcodedSettings::instance().serverApiUrl().toStdString().c_str());
    domainPtr_ = szDomain_;
    aresLibraryInit_.init();

    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel(QStringList(), options, optmask);
    {
        QMutexLocker locker(&mutex_);
        int status = ares_init_options(&channel_, &options, optmask);
        if (status != ARES_SUCCESS)
        {
            qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
            return;
        }
    }

    createOptionsForAresChannel(getCustomDnsIps(), options, optmask);
    {
        QMutexLocker locker(&mutex_);
        int status = ares_init_options(&channelCustomDns_, &options, optmask);
        if (status != ARES_SUCCESS)
        {
            qCDebug(LOG_BASIC) << "ares_init_options failed for custon dns:" << QString::fromStdString(ares_strerror(status));
            return;
        }
    }

    start(QThread::LowPriority);
}

DnsResolver::~DnsResolver()
{
    Q_ASSERT(bStopCalled_);
    this_ = NULL;
}

void DnsResolver::run()
{
    Debug::CrashHandlerForThread bind_crash_handler_to_this_thread;
    while (true)
    {
        QMutexLocker lockerWait(&mutexWait_);
        if (bNeedFinish_)
        {
            break;
        }

        bool b1, b2;
        {
            QMutexLocker locker(&mutex_);
            b1 = processChannel(channel_);
            b2 = processChannel(channelCustomDns_);
        }
        if (!b1 && !b2)
        {
            // wait for new request
            waitCondition_.wait(&mutexWait_);
        }
    }

    {
        QMutexLocker locker(&mutex_);
        ares_destroy(channel_);
        ares_destroy(channelCustomDns_);

        channel_= NULL;
        channelCustomDns_ = NULL;
    }

}

void DnsResolver::recreateCustomDnsChannel()
{
    mutex_.lock();
    if (channelCustomDns_)
    {
        ares_destroy(channelCustomDns_);
        channelCustomDns_ = NULL;
    }

    struct ares_options options;
    int optmask = 0;

    createOptionsForAresChannel(getCustomDnsIps(), options, optmask);

    int status = ares_init_options(&channelCustomDns_, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
        channelCustomDns_ = NULL;
    }

    mutex_.unlock();
}

QStringList DnsResolver::getCustomDnsIps()
{
    if (dnsPolicyType_ == DNS_TYPE_OS_DEFAULT)
    {
        return QStringList();
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

void DnsResolver::createOptionsForAresChannel(const QStringList &dnsIps, ares_options &options, int &optmask)
{
    memset(&options, 0, sizeof(options));
    optmask = 0;

    if (dnsIps.isEmpty())
    {
        optmask |= ARES_OPT_DOMAINS;
        optmask |= ARES_OPT_TRIES;
        options.ndomains = 1;
        options.domains = &domainPtr_;
        options.tries = 1;
    }
    else
    {
        optmask |= ARES_OPT_DOMAINS;
        optmask |= ARES_OPT_SERVERS;
        optmask |= ARES_OPT_TRIES;
        options.ndomains = 1;
        options.domains = &domainPtr_;
        options.tries = 1;

        Q_ASSERT(dnsIps.count() <= 2);
        struct sockaddr_in sa;

        ares_inet_pton(AF_INET, dnsIps[0].toStdString().c_str(), &(sa.sin_addr));
        dnsServers_[0] = sa.sin_addr;
        if (dnsIps.count() > 1)
        {
            ares_inet_pton(AF_INET, dnsIps[1].toStdString().c_str(), &(sa.sin_addr));
            dnsServers_[1] = sa.sin_addr;
        }
        options.nservers = dnsIps.count();
        options.servers = dnsServers_;
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
    tvp.tv_sec = 1;
    tvp.tv_usec = 0;

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
