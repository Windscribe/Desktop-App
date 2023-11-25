#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#ifdef Q_OS_WIN
#include <WinSock2.h>
#endif
#include "engine/networkaccessmanager/dnscache.h"

class TestDnsCache : public QObject
{
    Q_OBJECT

public:
    TestDnsCache();
    ~TestDnsCache();

private slots:
    void basicTest();
    void testCacheTimeout();

private:
    void delay(int ms);
};


TestDnsCache::TestDnsCache()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

TestDnsCache::~TestDnsCache()
{
}

void TestDnsCache::basicTest()
{
    DnsCache *dnsCache = new DnsCache(this);

    {
        QSignalSpy spy(dnsCache, &DnsCache::resolved);

        dnsCache->resolve("google.com", 0, false);

        spy.wait(10000);
        QCOMPARE(spy.count(), 1);

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY(arguments.at(0).toBool() == true);
        QVERIFY(arguments.at(1).toStringList().size() > 0);
        QVERIFY(arguments.at(2).toULongLong() == 0);
        QVERIFY(arguments.at(3).toBool() == false);
    }
    {
            QSignalSpy spy(dnsCache, &DnsCache::resolved);

            dnsCache->resolve("google.com", 1, false);

            if (spy.count() == 0)
            {
                spy.wait(10000);
            }
            QCOMPARE(spy.count(), 1);

            QList<QVariant> arguments = spy.takeFirst();
            QVERIFY(arguments.at(0).toBool() == true);
            QVERIFY(arguments.at(1).toStringList().size() > 0);
            QVERIFY(arguments.at(2).toULongLong() == 1);
            QVERIFY(arguments.at(3).toBool() == true);
        }

    {
        QSignalSpy spy(dnsCache, &DnsCache::resolved);

        dnsCache->resolve("google.com", 2, true);

        spy.wait(10000);
        QCOMPARE(spy.count(), 1);

        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY(arguments.at(0).toBool() == true);
        QVERIFY(arguments.at(1).toStringList().size() > 0);
        QVERIFY(arguments.at(2).toULongLong() == 2);
        QVERIFY(arguments.at(3).toBool() == false);
    }
}

void TestDnsCache::testCacheTimeout()
{
     DnsCache *dnsCache = new DnsCache(this, 3000, 10);
     {
         QSignalSpy spy(dnsCache, &DnsCache::resolved);

         QObject::connect(dnsCache, &DnsCache::resolved, this, [=](bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs)
         {
             QVERIFY(success == true);
         });

         dnsCache->resolve("google.com", 0, false);

         if (spy.count() == 0) { spy.wait(10000); }
         QCOMPARE(spy.count(), 1);
         QList<QVariant> arguments = spy.takeFirst();
         QVERIFY(arguments.at(3).toBool() == false);
     }
     {
         QSignalSpy spy(dnsCache, &DnsCache::resolved);

         dnsCache->resolve("google.com", 0, false);

         if (spy.count() == 0) { spy.wait(10000); }
         QCOMPARE(spy.count(), 1);
         QList<QVariant> arguments = spy.takeFirst();
         QVERIFY(arguments.at(3).toBool() == true);
     }
     delay(3100);
     {
         QSignalSpy spy(dnsCache, &DnsCache::resolved);

         dnsCache->resolve("google.com", 0, false);

         if (spy.count() == 0) { spy.wait(10000); }
         QCOMPARE(spy.count(), 1);
         QList<QVariant> arguments = spy.takeFirst();
         QVERIFY(arguments.at(3).toBool() == false);
     }
}


void TestDnsCache::delay(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

QTEST_MAIN(TestDnsCache)
#include "dnscache.test.moc"
