#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QVector>
#include "areslibraryinit.h"
#include "ares.h"
#include "dnsresolver_test.h"


// singleton
class DnsResolver : public QThread
{
    Q_OBJECT

public:
    static DnsResolver &instance()
    {
        static DnsResolver s;
        return s;
    }

    //void runTests();
    void stop();

    // if ips is empty, then use default OS DNS
    // sets the DNS servers for all subsequent requests
    void setDnsServers(const QStringList &ips);

    void lookup(const QString &hostname, void *userPointer);
    QStringList lookupBlocked(const QString &hostname);

private:
    explicit DnsResolver(QObject *parent = nullptr);
    virtual ~DnsResolver();

protected:
    virtual void run();

signals:
    void resolved(const QString &hostname, const QStringList &addresses, void *userPointer);

private:

    struct USER_ARG
    {
        void *userPointer;
        QString hostname;
    };

    struct USER_ARG_FOR_BLOCKED
    {
        QStringList ips;
    };

    struct ALLOCATED_DATA_FOR_OPTIONS
    {
#ifdef Q_OS_WIN
        QVector<IN_ADDR> dnsServers;
#else
        QVector<in_addr> dnsServers;
#endif
    };

    struct CHANNEL_INFO
    {
        ares_channel channel;
        ALLOCATED_DATA_FOR_OPTIONS *allocatedData;
    };

    AresLibraryInit aresLibraryInit_;
    bool bStopCalled_;
    QQueue<CHANNEL_INFO> queue_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool bNeedFinish_;
    QStringList dnsServers_;

    static DnsResolver *this_;

    QStringList getDnsIps();
    void createOptionsForAresChannel(const QStringList &dnsIps, struct ares_options &options, int &optmask, ALLOCATED_DATA_FOR_OPTIONS *allocatedData);
    static void callback(void *arg, int status, int timeouts, struct hostent *host);
    static void callbackForBlocked(void *arg, int status, int timeouts, struct hostent *host);
    // return false, if nothing to process more
    bool processChannel(ares_channel channel);
};

#endif // DNSRESOLVER_H
