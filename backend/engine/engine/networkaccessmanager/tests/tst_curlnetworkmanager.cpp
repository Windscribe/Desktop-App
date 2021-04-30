#include "tst_curlnetworkmanager.h"
#include "networkaccessmanager/curlnetworkmanager.h"
#include "dnsresolver/dnsrequest.h"
#include <QtTest>

TestCurlNetworkManager::TestCurlNetworkManager()
{
}

TestCurlNetworkManager::~TestCurlNetworkManager()
{
}

void TestCurlNetworkManager::test_get()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);
    NetworkRequest request(QUrl("https://httpbin.org/get"), 5000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    CurlReply *reply = manager->get(request, dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
}

void TestCurlNetworkManager::test_incorrect_get()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);
    NetworkRequest request(QUrl("https://httpbin.org/get_incorrect"), 1000, false);

    CurlReply *reply = manager->get(request, QStringList() << "222.222.222.222");

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(!reply->isSuccess());
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
}

void TestCurlNetworkManager::test_timeout_get()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);

    const int time1 = 1000;
    QElapsedTimer timer;
    timer.start();
    NetworkRequest request(QUrl("https://httpbin.org/get_incorrect"), time1, false);
    CurlReply *reply = manager->get(request, QStringList() << "222.222.222.222");

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(!reply->isSuccess());
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);

    QVERIFY(timer.elapsed() < time1 * 1.2);
}

void TestCurlNetworkManager::test_ssl_errors()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);

    NetworkRequest request(QUrl("https://expired.badssl.com/"), 10000, false);
    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    QList<QSignalSpy *> signalSpyes;
    {
        NetworkRequest request(QUrl("https://expired.badssl.com/"), 10000, false);
        CurlReply *reply = manager->get(request, dnsRequest.ips());
        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSSLError());
            reply->deleteLater();
        });
        signalSpyes << new QSignalSpy (reply, SIGNAL(finished()));
    }
    {
        NetworkRequest request(QUrl("https://wrong.host.badssl.com/"), 10000, false);
        CurlReply *reply = manager->get(request, dnsRequest.ips());
        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSSLError());
            reply->deleteLater();
        });
        signalSpyes << new QSignalSpy (reply, SIGNAL(finished()));
    }
    {
        NetworkRequest request(QUrl("https://self-signed.badssl.com/"), 10000, false);
        CurlReply *reply = manager->get(request, dnsRequest.ips());
        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSSLError());
            reply->deleteLater();
        });
        signalSpyes << new QSignalSpy (reply, SIGNAL(finished()));
    }
    {
        NetworkRequest request(QUrl("https://untrusted-root.badssl.com/"), 10000, false);
        CurlReply *reply = manager->get(request, dnsRequest.ips());
        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSSLError());
            reply->deleteLater();
        });
        signalSpyes << new QSignalSpy (reply, SIGNAL(finished()));
    }

    for (auto it : signalSpyes)
    {
        if (it->count() == 0)
            it->wait(20000);
        delete it;
    }
}

void TestCurlNetworkManager::test_iplist_get()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);
    NetworkRequest request(QUrl("https://httpbin.org/get"), 10000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    {
        CurlReply *reply = manager->get(request, QStringList() << dnsRequest.ips()[0] << "222.222.222.222");

        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSuccess());
            reply->deleteLater();
        });

        QSignalSpy signalFinished(reply, SIGNAL(finished()));
        signalFinished.wait(20000);
        QCOMPARE(signalFinished.count(), 1);
    }
    {
        CurlReply *reply = manager->get(request, QStringList() << "222.222.222.222" << dnsRequest.ips()[0]);

        QObject::connect(reply, &CurlReply::finished, this, [=]
        {
            QVERIFY(reply->isSuccess());
            reply->deleteLater();
        });

        QSignalSpy signalFinished(reply, SIGNAL(finished()));
        signalFinished.wait(20000);
        QCOMPARE(signalFinished.count(), 1);
    }
}

void TestCurlNetworkManager::test_post()
{
    CurlNetworkManager *manager = new CurlNetworkManager(this);
    NetworkRequest request(QUrl("https://postman-echo.com/post?hand=wave"), 5000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    CurlReply *reply = manager->post(request, "hand=wave", dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        qDebug() << reply->readAll();
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
}



void TestCurlNetworkManager::test_async()
{
    /*CurlNetworkManager *manager = new CurlNetworkManager(this);

    NetworkRequest request(QUrl("https://assets.totallyacdn.com/desktop/win/Windscribe.exe"), 5000, false);


    for (int i = 0; i < 100; ++i)
    {
        CurlReply *reply = manager->get(request, QStringList() << "104.25.98.39");
        QSignalSpy signalFinished(reply, SIGNAL(finished()));
        QSignalSpy signalProgress(reply, SIGNAL(progress(qint64,qint64)));
    }

    //QSignalSpy signalFinished(reply, SIGNAL(finished()));
    //QSignalSpy signalProgress(reply, SIGNAL(progress(qint64,qint64)));

    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);

    QVERIFY(signalProgress.count() >= 1);
    */
}

