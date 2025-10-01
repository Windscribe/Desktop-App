#pragma once

#include <QThread>

namespace NetworkUtils {

// Implemented on the basis of c-ares
// Do not use QDnsLookup here, as it can block program termination for a long time
class DnsChecker : public QThread
{
    Q_OBJECT
public:
    explicit DnsChecker(QObject *parent = nullptr);
    ~DnsChecker();

    void checkAvailability(const QString &dnsAddress = "127.0.0.1", uint port = 53, int timeoutMs = 500);
    static bool checkAvailabilityBlocking(const QString &dnsAddress = "127.0.0.1", uint port = 53, int timeoutMs = 500);

signals:
    void dnsCheckCompleted(bool available);

protected:
    void run() override;

private:
    QString dnsAddress_;
    uint port_;
    int timeoutMs_;
    std::atomic_bool bFinish_ = false;
};

} // namespace NetworkUtils
