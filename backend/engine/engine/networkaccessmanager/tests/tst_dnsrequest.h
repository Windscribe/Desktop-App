#ifndef TESTDNSREQUEST_H
#define TESTDNSREQUEST_H

#include <QObject>
#include <QMutex>

class TestDnsRequest : public QObject
{
    Q_OBJECT

public:
    TestDnsRequest();
    ~TestDnsRequest();

private slots:
    void test_async();
    void test_blocked();
    void test_incorrect();
    void test_subdomain();
    void test_timeout();
    void test_timeout_blocked();

private:
    QStringList domains_;
    QMutex mutex_;

    QString getRandomDomain();
    int generateIntegerRandom(const int &min, const int &max);
};


#endif // TESTDNSREQUEST_H
