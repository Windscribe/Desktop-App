#include "dnsresolver.h"
#include "dnsutils.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "ares.h"

#include <QRunnable>

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/select.h>
#endif

namespace {

std::atomic_bool g_FinishAll = false;

#ifdef Q_OS_WIN
        typedef QVector<IN_ADDR> DnsAddrs;
#else
        typedef QVector<in_addr> DnsAddrs;
#endif

struct UserArg
{
    QStringList ips;
    int errorCode = ARES_ECANCELLED;
};

class LookupJob : public QRunnable
{
public:
     LookupJob(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs) :
        hostname_(hostname),
        object_(object),
        dnsServers_(dnsServers),
        timeoutMs_(timeoutMs),
        elapsedMs_(0)
    {
    }

    void run() override
    {
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        while (errorCode_ != ARES_SUCCESS && elapsedTimer.elapsed() <= timeoutMs_) {

            // fill dns addresses
            DnsAddrs dnsAddrs;
            const QStringList dnsList = getDnsIps(dnsServers_);
            dnsAddrs.reserve(dnsList.count());
            for (const auto &dnsIp : dnsList) {
                struct sockaddr_in sa;
                ares_inet_pton(AF_INET, dnsIp.toStdString().c_str(), &(sa.sin_addr));
                dnsAddrs.push_back(sa.sin_addr);
            }

            // create channel and set options
            ares_channel channel;
            struct ares_options options;
            int optmask = 0;
            createOptionsForAresChannel(timeoutMs_, options, optmask, dnsAddrs);
            int status = ares_init_options(&channel, &options, optmask);
            if (status != ARES_SUCCESS) {
                qCDebug(LOG_BASIC) << "ares_init_options failed:" << QString::fromStdString(ares_strerror(status));
                return;
            }

            UserArg userArg;
            ares_gethostbyname(channel, hostname_.toStdString().c_str(), AF_INET, callback, &userArg);

            // process loop
            timeval tv;
            while (!g_FinishAll) {
                fd_set readers, writers;
                FD_ZERO(&readers);
                FD_ZERO(&writers);
                int nfds = ares_fds(channel, &readers, &writers);
                if (nfds == 0)
                    break;
                timeval *tvp = ares_timeout(channel, NULL, &tv);
                select(nfds, &readers, &writers, NULL, tvp);
                ares_process(channel, &readers, &writers);
                if (elapsedTimer.elapsed() > timeoutMs_) {
                    userArg.errorCode = ARES_ETIMEOUT;
                    break;
                }
            }
            ares_destroy(channel);

            ips_ = userArg.ips;
            errorCode_ = userArg.errorCode;
            elapsedMs_ = elapsedTimer.elapsed();
        }

        if (object_) {
            bool bSuccess = QMetaObject::invokeMethod(object_.get(), "onResolved",
                            Qt::QueuedConnection, Q_ARG(QStringList, ips_), Q_ARG(int, errorCode_), Q_ARG(qint64, elapsedMs_));
            WS_ASSERT(bSuccess);
        }
    }

    QStringList ips() const { return ips_; }
    int errorCode() const { return errorCode_; }
    qint64 elapsedMs() const { return elapsedMs_; }

private:
    static constexpr int kTimeoutMs = 2000;
    static constexpr int kTries = 1;
    QString hostname_;
    QSharedPointer<QObject> object_;
    QStringList dnsServers_;
    int timeoutMs_;
    qint64 elapsedMs_;

    QStringList ips_;
    int errorCode_ = ARES_ECANCELLED;

    QStringList getDnsIps(const QStringList &ips)
    {
        if (ips.isEmpty()) {
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

    void createOptionsForAresChannel(int timeoutMs, ares_options &options, int &optmask, DnsAddrs &dnsAddrs)
    {
        memset(&options, 0, sizeof(options));

        if (dnsAddrs.isEmpty()) {
            optmask = ARES_OPT_TRIES | ARES_OPT_TIMEOUTMS;
            options.tries = kTries;
            options.timeout = kTimeoutMs;
        } else {
            optmask = ARES_OPT_TRIES | ARES_OPT_SERVERS | ARES_OPT_TIMEOUTMS;
            options.tries = kTries;
            options.timeout = kTimeoutMs;
            options.nservers = dnsAddrs.count();
            options.servers = &(dnsAddrs[0]);
        }
    }

    static void callback(void *arg, int status, int timeouts, struct hostent *host)
    {
        Q_UNUSED(timeouts);
        UserArg *userArg = static_cast<UserArg *>(arg);

        if (status == ARES_SUCCESS) {
            QStringList addresses;
            for (char **p = host->h_addr_list; *p; p++) {
                char addr_buf[46] = "??";

                ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
                addresses << QString::fromStdString(addr_buf);
            }
            userArg->ips = addresses;
        } else {
            userArg->ips.clear();
        }
        userArg->errorCode = status;
    }
};

} // namespace

DnsResolver::DnsResolver(QObject *parent) : QObject(parent)
{
    aresLibraryInit_.init();
    threadPool_ = new QThreadPool(this);
}

DnsResolver::~DnsResolver()
{
    qCDebug(LOG_BASIC) << "Stopping DnsResolver";
    g_FinishAll = true;
    threadPool_->waitForDone();
    qCDebug(LOG_BASIC) << "DnsResolver stopped";
}

void DnsResolver::lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs)
{
    LookupJob *job = new LookupJob(hostname, object, dnsServers, timeoutMs);
    threadPool_->start(job);
}

QStringList DnsResolver::lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, int *outErrorCode)
{
    LookupJob job(hostname, nullptr, dnsServers, timeoutMs);
    job.run();
    if (outErrorCode) {
        *outErrorCode = job.errorCode();
    }
    return job.ips();
}
