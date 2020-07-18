#ifndef DNSRESOLVER_H
#define DNSRESOLVER_H

#include <QHostInfo>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include "areslibraryinit.h"
#include "ares.h"
#include "engine/types/types.h"

class DnsResolver : public QThread
{
    Q_OBJECT

public:
    static DnsResolver &instance()
    {
        static DnsResolver s;
        return s;
    }

    void init();
    void stop();
    void setDnsPolicy(DNS_POLICY_TYPE dnsPolicyType);
    void recreateDefaultDnsChannel();
    void lookup(const QString &hostname, bool bUseCustomDns, void *userPointer, quint64 userId);

    QHostInfo lookupBlocked(const QString &hostname, bool bUseCustomDns);


private:
    explicit DnsResolver(QObject *parent = nullptr);
    virtual ~DnsResolver();

protected:
    virtual void run();

signals:
    void resolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer, quint64 userId);

private:

    struct USER_ARG
    {
        void *userPointer;
        quint64 userId;
        QString hostname;
    };

    struct USER_ARG_FOR_BLOCKED
    {
        QHostInfo ha;
    };

    bool bStopCalled_;
    QMutex mutex_;
    QMutex mutexWait_;
    QWaitCondition waitCondition_;
    bool bNeedFinish_;
    AresLibraryInit aresLibraryInit_;
    ares_channel channel_;
    ares_channel channelCustomDns_;

    DNS_POLICY_TYPE dnsPolicyType_;
    char szDomain_[128];
    char *domainPtr_;
#ifdef Q_OS_WIN
    IN_ADDR dnsServers_[2];
#else
    in_addr dnsServers_[2];
#endif


    static DnsResolver *this_;

    void recreateCustomDnsChannel();
    QStringList getCustomDnsIps();

    void createOptionsForAresChannel(const QStringList &dnsIps, struct ares_options &options, int &optmask);

    static void callback(void *arg, int status, int timeouts, struct hostent *host);
    static void callbackForBlocked(void *arg, int status, int timeouts, struct hostent *host);
    // return false, if nothing to process more
    bool processChannel(ares_channel channel);
};

#endif // DNSRESOLVER_H
