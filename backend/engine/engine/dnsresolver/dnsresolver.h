#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>
#include "areslibraryinit.h"
#include "ares.h"
#include "dnsresolver_test.h"

// singleton for dns requests. Do not use it directly. Use DnsLookup instead
class DnsResolver : public QThread
{
    Q_OBJECT

public:
    static DnsResolver &instance()
    {
        static DnsResolver s;
        return s;
    }

    void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers);
    QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers);

private:
    explicit DnsResolver(QObject *parent = nullptr);
    virtual ~DnsResolver();

protected:
    virtual void run();

private:

    struct USER_ARG
    {
        QSharedPointer<QObject> object;
        QString hostname;
    };

    struct USER_ARG_FOR_BLOCKED
    {
        QStringList ips;
    };

    struct REQUEST_INFO
    {
        QString hostname;
        QStringList dnsServers;
        QSharedPointer<QObject> object;
    };

    struct CHANNEL_INFO
    {
        ares_channel channel;

#ifdef Q_OS_WIN
        QVector<IN_ADDR> dnsServers;
#else
        QVector<in_addr> dnsServers;
#endif
    };

    AresLibraryInit aresLibraryInit_;
    bool bStopCalled_;
    QQueue<REQUEST_INFO> queue_;

    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool bNeedFinish_;

    static DnsResolver *this_;

    QStringList getDnsIps(const QStringList &ips);
    void createOptionsForAresChannel(const QStringList &dnsIps, struct ares_options &options, int &optmask, CHANNEL_INFO *channelInfo);
    static void callback(void *arg, int status, int timeouts, struct hostent *host);
    static void callbackForBlocked(void *arg, int status, int timeouts, struct hostent *host);
    // return false, if nothing to process more
    bool processChannel(ares_channel channel);
    bool initChannel(const REQUEST_INFO &ri, CHANNEL_INFO &outChannelInfo);
};

#endif // DNSRESOLVER_H
