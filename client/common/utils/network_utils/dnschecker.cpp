#include "dnschecker.h"

#include <QEventLoop>
#include <ares.h>

#include "../ws_assert.h"
#include "../log/categories.h"

namespace NetworkUtils {

void caresCallback(void *arg, int status, int timeouts, struct ares_addrinfo *res)
{
    DnsChecker *this_ = (DnsChecker *)arg;
    emit this_->dnsCheckCompleted(status == ARES_SUCCESS);
}

DnsChecker::DnsChecker(QObject *parent) : QThread(parent)
{
    int status = ares_library_init(ARES_LIB_INIT_ALL);
    WS_ASSERT(status == ARES_SUCCESS);

    if (!ares_threadsafety()) {
        WS_ASSERT(false);
    }
}

DnsChecker::~DnsChecker()
{
    bFinish_ = true;
    wait(ULONG_MAX);
    ares_library_cleanup();
}

void DnsChecker::checkAvailability(const QString &dnsAddress, uint port, int timeoutMs)
{
    WS_ASSERT(!isRunning());
    dnsAddress_ = dnsAddress;
    port_ = port;
    timeoutMs_ = timeoutMs;
    start(LowPriority);
}

bool DnsChecker::checkAvailabilityBlocking(const QString &dnsAddress, uint port, int timeoutMs)
{
    QEventLoop eventLoop;
    DnsChecker dnsChecker;
    bool result = false;

    QObject::connect(&dnsChecker, &DnsChecker::dnsCheckCompleted,
                     [&eventLoop, &result](bool available) {
                         result = available;
                         eventLoop.quit();
                     });
    dnsChecker.checkAvailability(dnsAddress, port, timeoutMs);
    eventLoop.exec();

    return result;
}

void DnsChecker::run()
{
    ares_channel channel;

    struct ares_options options;
    int optmask;

    memset(&options, 0, sizeof(options));
    optmask = ARES_OPT_TRIES | ARES_OPT_TIMEOUTMS | ARES_OPT_MAXTIMEOUTMS | ARES_OPT_EVENT_THREAD;
    options.tries = 2;
    options.timeout = timeoutMs_;
    options.maxtimeout = timeoutMs_;
    options.evsys = ARES_EVSYS_DEFAULT;

    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS) {
        qCritical(LOG_BASIC) << "DnsChecker::run(), ares_init_options failed";
        emit dnsCheckCompleted(false);
        return;
    }
    QString serverStr = dnsAddress_ + ":" + QString::number(port_);
    status = ares_set_servers_csv(channel, serverStr.toStdString().c_str());
    if (status != ARES_SUCCESS) {
        qCritical(LOG_BASIC) << "DnsChecker::run(), ares_set_servers_csv failed";
        emit dnsCheckCompleted(false);
        return;
    }

    struct ares_addrinfo_hints hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags  = ARES_AI_CANONNAME;
    ares_getaddrinfo(channel,  "windscribe.com", NULL, &hints, caresCallback, this);

    do {
        status = ares_queue_wait_empty(channel, 10);    // 10 ms
    } while (status != ARES_SUCCESS && !bFinish_);

    ares_destroy(channel);
}

} // namespace NetworkUtils
