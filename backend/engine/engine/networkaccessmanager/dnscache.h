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

    void resolve(const QString &hostname, quint64 id, bool bypassCache = false);
    void notifyFinished(quint64 id);

signals:
    void resolved(bool success, const QStringList &ips, quint64 id, bool bFromCache);
    void ipsInCachChanged(const QStringList &ips);

private slots:
    void onDnsRequestFinished();
    void onTimer();

private:

    static constexpr int CACHE_TIMEOUT = 60000;        // 1 min
    static constexpr int REVIEW_CACHE_PERIOD = 1000;        // 1 sec

    struct CacheItem
    {
        qint64 time;
        QStringList ips;
        int usages;

        CacheItem() : time(0), usages(0) {}
    };

    QMap<QString, CacheItem > cacheByHostname_;
    QMap<quint64, CacheItem *> cacheById_;

    /*QSet<QString> resolvedIps_;*/

    struct PendingRequest
    {
        QString hostname;
        quint64 id;
    };

    QSet<QString> resolvingHostsInProgress_;
    QMap<QString, QList<quint64>> pendingRequests_;

    void checkForNewIps(const QStringList &newIps);
    void checkForNewIps(const QString &ip);
};

#endif // DNSCACHE_H
