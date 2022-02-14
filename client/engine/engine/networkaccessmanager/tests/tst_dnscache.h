#ifndef TESTDNSCACHE_H
#define TESTDNSCACHE_H

#include <QObject>
#include <QMutex>

class TestDnsCache : public QObject
{
    Q_OBJECT

public:
    TestDnsCache();
    ~TestDnsCache();

private slots:
    void basicTest();
    void testCacheTimeout();
    void testWhitelist();

private:
    void delay(int ms);
};


#endif // TESTDNSCACHE_H
