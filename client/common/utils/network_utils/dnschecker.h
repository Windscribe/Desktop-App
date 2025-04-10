#pragma once

#include <QDnsLookup>
#include <QObject>
#include <QString>
#include <QTimer>

namespace NetworkUtils {

class DnsChecker : public QObject
{
    Q_OBJECT
public:
    explicit DnsChecker(QObject *parent = nullptr);
    ~DnsChecker();

    void checkAvailability(const QString &dnsAddress = "127.0.0.1", uint port = 53, int timeoutMs = 500);
    static bool checkAvailabilityBlocking(const QString &dnsAddress = "127.0.0.1", uint port = 53, int timeoutMs = 500);

signals:
    void dnsCheckCompleted(bool available);

private slots:
    void handleLookupFinished();
    void handleTimeout();

private:
    QDnsLookup *dnsLookup_;
    QTimer timer_;
};

} // namespace NetworkUtils
