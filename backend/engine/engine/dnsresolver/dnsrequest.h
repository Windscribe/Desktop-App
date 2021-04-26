#ifndef DNSREQUEST_H
#define DNSREQUEST_H

#include <QMutex>
#include <QObject>


// Required for managing the lifetime of an object in shared pointer
class DnsRequestPrivate : public QObject
{
    Q_OBJECT

signals:
    void resolved(const QStringList &ips);

private slots:
    void onResolved(const QStringList &ips);
};


class DnsRequest : public QObject
{
    Q_OBJECT
public:
    explicit DnsRequest(QObject *parent, const QString &hostname);

    QStringList ips() const;
    QString hostname() const;
    bool isError() const;
    void lookup();
    void lookupBlocked();

signals:
    void finished();

private slots:
    void onResolved(const QStringList &ips);

private:
    QString hostname_;
    QStringList ips_;
};

#endif // DNSREQUEST_H
