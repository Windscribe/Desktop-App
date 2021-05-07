#ifndef DNSRESOLVER_TEST_H
#define DNSRESOLVER_TEST_H

#include <QMutex>
#include <QObject>
#include <QVector>


class DnsResolver_test : public QObject
{
    Q_OBJECT
public:
    explicit DnsResolver_test(QObject *parent);
    virtual ~DnsResolver_test();

    void runTests();
    void stop();

private slots:
    void onResolved();

private:
    QVector<QString> domains_;
    std::atomic<bool> bStopped_;
    QMutex mutex_;
    int resolved_;
    QString getRandomDomain();
    int generateIntegerRandom(const int &min, const int &max);
};

#endif // DNSRESOLVER_TEST_H
