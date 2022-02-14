#ifndef DNSCACHE2_H
#define DNSCACHE2_H

#include <QMap>
#include <QObject>
#include <QHostInfo>

class DnsCache2 : public QObject
{
    Q_OBJECT
public:
    class Usages;

    explicit DnsCache2(QObject *parent, int cacheTimeoutMs = 60000, int reviewCacheIntervalMs = 1000);
    virtual ~DnsCache2();

    void resolve(const QString &hostname, quint64 id, bool bypassCache = false, const QStringList &dnsServers = QStringList(), int timeoutMs = 5000);
    void notifyFinished(quint64 id);

    const QSet<QString> &whitelistIps() const;

signals:
    void resolved(bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs);
    void whitelistIpsChanged(const QSet<QString> &ips);

private slots:
    void onDnsRequestFinished();
    void onTimer();
    void checkForNewIps();

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
    Usages *usages_;
    QSet<QString> lastWhitelistIps_;
    int cacheTimeoutMs_;

};

#endif // DNSCACHE2_H
