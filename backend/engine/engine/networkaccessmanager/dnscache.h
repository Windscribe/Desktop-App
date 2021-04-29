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

    void resolve(const QString &hostname, void *userData);
    void resolve(const QString &hostname, int cacheTimeout, void *userData);

signals:
    void resolved(bool success, void *userData, const QStringList &ips);
    void ipsInCachChanged(const QStringList &ips);

private slots:
    void onDnsRequestFinished();
    void onTimer();

private:

    struct ResolvedHostInfo
    {
        qint64 time;
        QStringList ips;
    };

    static constexpr int CACHE_TIMEOUT = 60000;        // 1 min
    static constexpr int REVIEW_CACHE_PERIOD = 1000;        // 1 sec

    QMap<QString, ResolvedHostInfo> cache_;
    QSet<QString> resolvedIps_;

    struct PendingHost
    {
        QString hostname;
        void *userData;
    };

    QSet<QString> resolvingHostsInProgress_;
    QVector<PendingHost> pendingHosts_;

    void checkForNewIps(const QStringList &newIps);
    void checkForNewIps(const QString &ip);
};

#endif // DNSCACHE_H
