#ifndef DNSCACHE_H
#define DNSCACHE_H

#include <QMap>
#include <QObject>
#include <QHostInfo>

class DnsCache : public QObject
{
    Q_OBJECT
public:
    explicit DnsCache(QObject *parent = 0);
    virtual ~DnsCache();

    void resolve(const QString &hostname, void *userData, qint64 requestStartTime);
    void resolve(const QString &hostname, int cacheTimeout, void *userData, qint64 requestStartTime);

signals:
    void resolved(bool success, void *userData, qint64 requestStartTime, const QStringList &ips);
    void ipsInCachChanged(const QStringList &ips);

private slots:
    void onDnsRequestFinished();

private:

    struct ResolvedHostInfo
    {
        qint64 time;
        QStringList ips;
    };

    static constexpr int CACHE_TIMEOUT = 60000;        // 1 min
    QMap<QString, ResolvedHostInfo> resolvedHosts_;
    QSet<QString> resolvedIps_;

    struct PendingResolvingHosts
    {
        QString hostname;
        void *userData;
        qint64 requestStartTime;
    };

    QSet<QString> resolvingHostsInProgress_;
    QVector<PendingResolvingHosts> pendingHosts_;

    void checkForNewIps(const QStringList &newIps);
    void checkForNewIps(const QString &ip);
};

#endif // DNSCACHE_H
