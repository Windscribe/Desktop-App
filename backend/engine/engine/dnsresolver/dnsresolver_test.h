#ifndef DNSRESOLVER_TEST_H
#define DNSRESOLVER_TEST_H

#include <QHostInfo>
#include <QMutex>


class DnsResolver_test : public QObject
{
    Q_OBJECT
public:
    explicit DnsResolver_test(QObject *parent);

    void runTests();
    void stop();

private slots:
    void onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer);

private:
    QVector<QString> domains_;
    std::atomic<bool> bStopped_;
    QMutex mutex_;
    QString getRandomDomain();
};

#endif // DNSRESOLVER_TEST_H
