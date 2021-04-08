#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QHostInfo>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QVector>
#include "areslibraryinit.h"
#include "ares.h"
#include "engine/types/types.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"
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

    void runTests();
    void stop();

    void setDnsPolicy(DNS_POLICY_TYPE dnsPolicyType);
    void setUseCustomDns(bool bUseCustomDns);

    void lookup(const QString &hostname, void *userPointer);
    QHostInfo lookupBlocked(const QString &hostname);

private:
    explicit DnsResolver(QObject *parent = nullptr);
    virtual ~DnsResolver();

protected:
    virtual void run();

signals:
    void resolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer);

private:

    struct USER_ARG
    {
        void *userPointer;
        QString hostname;
    };

    struct USER_ARG_FOR_BLOCKED
    {
        QHostInfo ha;
    };

    struct ALLOCATED_DATA_FOR_OPTIONS
    {
        char szDomain[128];
        char *domainPtr;
#ifdef Q_OS_WIN
        QVector<IN_ADDR> dnsServers;
#else
        QVector<in_addr> dnsServers;
#endif
        ALLOCATED_DATA_FOR_OPTIONS();
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
    DNS_POLICY_TYPE dnsPolicyType_;
    std::atomic_bool isUseCustomDns_;

    DnsResolver_test *test_;

    static DnsResolver *this_;

    QStringList getCustomDnsIps();
    void createOptionsForAresChannel(const QStringList &dnsIps, struct ares_options &options, int &optmask, ALLOCATED_DATA_FOR_OPTIONS *allocatedData);
    static void callback(void *arg, int status, int timeouts, struct hostent *host);
    static void callbackForBlocked(void *arg, int status, int timeouts, struct hostent *host);
    // return false, if nothing to process more
    bool processChannel(ares_channel channel);
};

#endif // DNSRESOLVER_H
