#pragma once

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>

#include "idnsresolver.h"

class DnsRequest : public QObject
{
    Q_OBJECT
public:
    explicit DnsRequest(QObject *parent, const QString &hostname, const QStringList &dnsServers = QStringList(), int timeoutMs = 5000);
    virtual ~DnsRequest();

    void lookup();
    void lookupBlocked();

    QStringList ips() const;
    QString hostname() const;
    bool isError() const;
    QString errorString() const;
    qint64 elapsedMs() const;

signals:
    void finished();

private slots:
    void onResolved(const QStringList &ips, const QString &error, qint64 elapsedMs);

private:
    IDnsResolver &dnsResolver_;
    QString hostname_;
    QStringList ips_;
    QStringList dnsServers_;
    int timeoutMs_;
    QString error_;
    qint64 elapsedMs_;
    // this object is shared with DnsResolver object
    // we need to store it here otherwise it may be deleted by DnsResolver before the signal finished() is emitted
    QSharedPointer<QObject> privateDnsRequestObject_;
};

// Required for managing the lifetime of an object in shared pointer
class DnsRequestPrivate : public QObject
{
    Q_OBJECT

signals:
    void resolved(const QStringList &ips,  const QString &error, qint64 elapsedMs);

private slots:
    void onResolved(const QStringList &ips,  const QString &error, qint64 elapsedMs);
};
