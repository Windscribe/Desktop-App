#pragma once

#include <QMap>
#include <QObject>

class DnsCache : public QObject
{
    Q_OBJECT
public:
    class Usages;

    explicit DnsCache(QObject *parent, int cacheTimeoutMs = 60000, int reviewCacheIntervalMs = 1000);
    virtual ~DnsCache();

    void resolve(const QString &hostname, quint64 id, bool bypassCache = false, const QStringList &dnsServers = QStringList(), int timeoutMs = 5000);

signals:
    void resolved(bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs);

private slots:
    void onDnsRequestFinished();
    void onTimer();

private:
    struct CacheItem
    {
        qint64 time;
        QStringList ips;

        CacheItem() : time(0) {}
    };

    struct PendingRequest
    {
        QString hostname;
        quint64 id;
    };

    QMap<QString, CacheItem> cache_;
    int cacheTimeoutMs_;

};

