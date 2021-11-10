#include <QtTest>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <WinSock2.h>
#include "tst_dnscache.h"
#include "networkaccessmanager/dnscache.h"

TestDnsCache::TestDnsCache()
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
}

TestDnsCache::~TestDnsCache()
{

}

void TestDnsCache::basicTest()
{
    DnsCache *dnsCache = new DnsCache(this);

    {
        QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));

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
        QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));

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
        QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));

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
         QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));

         QObject::connect(dnsCache, &DnsCache::resolved, this, [=](bool success, const QStringList &ips, quint64 id, bool bFromCache)
         {
             dnsCache->notifyFinished(id);
         });

         dnsCache->resolve("google.com", 0, false);

         if (spy.count() == 0) { spy.wait(10000); }
         QCOMPARE(spy.count(), 1);
         QList<QVariant> arguments = spy.takeFirst();
         QVERIFY(arguments.at(3).toBool() == false);
     }
     delay(3100);
     {
         QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));

         dnsCache->resolve("google.com", 0, false);

         if (spy.count() == 0) { spy.wait(10000); }
         QCOMPARE(spy.count(), 1);
         QList<QVariant> arguments = spy.takeFirst();
         QVERIFY(arguments.at(3).toBool() == false);
     }
}

void TestDnsCache::testWhitelist()
{
    int state = 0;

    DnsCache *dnsCache = new DnsCache(this, 3000, 10);
    QObject::connect(dnsCache, &DnsCache::resolved, this, [&state, dnsCache](bool success, const QStringList &ips, quint64 id, bool bFromCache)
    {
        state++;
    });
    QObject::connect(dnsCache, &DnsCache::whitelistIpsChanged, this, [&state, dnsCache](const QSet<QString> &ips)
    {
        if (state == 0)
        {
            QVERIFY(ips.count() != 0);
            QString ip = *ips.begin();
            QVERIFY(ip == "1.2.3.4" || ip == "1.2.3.5");
        }
        else if (state == 1)
        {
            QSet<QString> i;
            i << "1.2.3.4";
            i << "1.2.3.5";
            QVERIFY(i == ips);
        }
        else if (state == 2)
        {
            QVERIFY(ips.count() != 0);
            QString ip = *ips.begin();
            QVERIFY(ip == "1.2.3.4" || ip == "1.2.3.5");
        }
        else if (state == 3)
        {
            QVERIFY(ips.count() == 0);
        }
    });

    QSignalSpy spy(dnsCache, SIGNAL(resolved(bool, QStringList, quint64, bool)));
    dnsCache->resolve("1.2.3.4", 0, false);
    dnsCache->resolve("1.2.3.5", 1, false);
    while (spy.count() != 2) { spy.wait(10000); }
    QCOMPARE(spy.count(), 2);

    delay(3100);

    QSignalSpy spy2(dnsCache, SIGNAL(whitelistIpsChanged(QSet<QString>)));
    dnsCache->notifyFinished(0);
    spy2.wait(10000);
    state++;
    dnsCache->notifyFinished(1);
    spy2.wait(10000);
}


void TestDnsCache::delay(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

