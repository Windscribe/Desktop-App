#ifndef DNSREQUEST_H
#define DNSREQUEST_H

#include <QMutex>
#include <QObject>

// Required for managing the lifetime of an object in shared pointer
class DnsRequestPrivate : public QObject
{
    Q_OBJECT

signals:
    void resolved(const QStringList &ips, int aresErrorCode);

private slots:
    void onResolved(const QStringList &ips, int aresErrorCode);
};

class DnsRequest : public QObject
{
    Q_OBJECT
public:
    explicit DnsRequest(QObject *parent, const QString &hostname, const QStringList &dnsServers, int timeoutMs = 5000);
    virtual ~DnsRequest();

    QStringList ips() const;
    QString hostname() const;
    bool isError() const;
    QString errorString();
    void lookup();
    void lookupBlocked();

signals:
    void finished();

private slots:
    void onResolved(const QStringList &ips, int aresErrorCode);

private:
    QString hostname_;
    QStringList ips_;
    QStringList dnsServers_;
    int timeoutMs_;
    int aresErrorCode_;
};

#endif // DNSREQUEST_H
