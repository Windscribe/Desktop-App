#include "curlnetworkmanager.test.h"
#include "engine/dnsresolver/dnsrequest.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>

TestCurlNetworkManager::TestCurlNetworkManager()
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
}

TestCurlNetworkManager::~TestCurlNetworkManager()
{
}

void TestCurlNetworkManager::test_delete_manager()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 10000, false);
    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    int progressCalled = 0;

    CurlReply *reply = manager->get(request, dnsRequest.ips());
    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(false);
    });
    CurlReply *reply2 = manager->get(request, dnsRequest.ips());
    QObject::connect(reply2, &CurlReply::finished, this, [=]
    {
        QVERIFY(false);
    });

    delete reply2;
    manager->deleteLater();

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(1000);
    QCOMPARE(signalFinished.count(), 0);
}

void TestCurlNetworkManager::test_get()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 10000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    int progressCalled = 0;

    CurlReply *reply = manager->get(request, dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &errCode);
        QVERIFY(errCode.error == QJsonParseError::NoError && doc.isObject());
        QVERIFY(doc.object().contains("args"));
        QVERIFY(doc.object()["args"].isObject());
        QVERIFY(doc.object()["args"].toObject()["foo1"].toString() == "bar1");
        QVERIFY(doc.object()["args"].toObject()["foo2"].toString() == "bar2");

        reply->deleteLater();
    });

    QObject::connect(reply, &CurlReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });


    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestCurlNetworkManager::test_readall_get()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 10000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    QByteArray arr;
    CurlReply *reply = manager->get(request, dnsRequest.ips());

    QObject::connect(reply, &CurlReply::readyRead, this, [&arr, reply]
    {
        QByteArray newData = reply->readAll();
        arr.append(newData);
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);

    QVERIFY(reply->isSuccess());
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    QVERIFY(errCode.error == QJsonParseError::NoError && doc.isObject());
    QVERIFY(doc.object().contains("args"));
    QVERIFY(doc.object()["args"].isObject());
    QVERIFY(doc.object()["args"].toObject()["foo1"].toString() == "bar1");
    QVERIFY(doc.object()["args"].toObject()["foo2"].toString() == "bar2");

    reply->deleteLater();
}

void TestCurlNetworkManager::test_incorrect_get()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
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
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);

    const int time1 = 1000;
    QElapsedTimer timer;
    timer.start();
    NetworkRequest request(QUrl("https://httpbin.org/get_incorrect"), time1, false);
    CurlReply *reply = manager->get(request, QStringList() << "222.222.222.222");

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(!reply->isSuccess());
        qDebug() << reply->errorString();
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);

    QVERIFY(timer.elapsed() < time1 * 1.2);
}

void TestCurlNetworkManager::test_ssl_errors()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);

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

void TestCurlNetworkManager::test_ignore_ssl_error()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);

    NetworkRequest request(QUrl("https://expired.badssl.com/"), 10000, false);
    request.setIgnoreSslErrors(true);
    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);
    CurlReply *reply = manager->get(request, dnsRequest.ips());
    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(!reply->isSSLError());
        reply->deleteLater();
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);

    QVERIFY(signalFinished.count() == 1);
}

void TestCurlNetworkManager::test_iplist_get()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("http://neverssl.com"), 20000, false);
    request.setIgnoreSslErrors(true);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();

    QVERIFY(dnsRequest.ips().count() > 0);

    {
        CurlReply *reply = manager->get(request, QStringList() << dnsRequest.ips() << "222.222.222.222");

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
        CurlReply *reply = manager->get(request, QStringList() << "222.222.222.222" << dnsRequest.ips());

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
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/post?hand=wave"), 20000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    int progressCalled = 0;
    CurlReply *reply = manager->post(request, "postdata=testtest", dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &errCode);
        QVERIFY(errCode.error == QJsonParseError::NoError && doc.isObject());
        QVERIFY(doc.object().contains("args"));
        QVERIFY(doc.object()["args"].isObject());
        QVERIFY(doc.object()["args"].toObject()["hand"].toString() == "wave");

        QVERIFY(doc.object().contains("json"));
        QVERIFY(doc.object()["json"].isObject());
        QVERIFY(doc.object()["json"].toObject()["postdata"].toString() == "testtest");

        reply->deleteLater();
    });

    QObject::connect(reply, &CurlReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestCurlNetworkManager::test_put()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/put?hand=wave"), 20000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    int progressCalled = 0;
    CurlReply *reply = manager->put(request, "putdata=testtest", dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &errCode);
        QVERIFY(errCode.error == QJsonParseError::NoError && doc.isObject());
        QVERIFY(doc.object().contains("args"));
        QVERIFY(doc.object()["args"].isObject());
        QVERIFY(doc.object()["args"].toObject()["hand"].toString() == "wave");

        QVERIFY(doc.object().contains("json"));
        QVERIFY(doc.object()["json"].isObject());
        QVERIFY(doc.object()["json"].toObject()["putdata"].toString() == "testtest");

        reply->deleteLater();
    });

    QObject::connect(reply, &CurlReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestCurlNetworkManager::test_delete()
{
    CurlNetworkManager2 *manager = new CurlNetworkManager2(this);
    NetworkRequest request(QUrl("https://postman-echo.com/delete?hand=wave"), 20000, false);

    DnsRequest dnsRequest(this, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    int progressCalled = 0;
    CurlReply *reply = manager->deleteResource(request,  dnsRequest.ips());

    QObject::connect(reply, &CurlReply::finished, this, [=]
    {
        QVERIFY(reply->isSuccess());
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &errCode);
        QVERIFY(errCode.error == QJsonParseError::NoError && doc.isObject());
        QVERIFY(doc.object().contains("args"));
        QVERIFY(doc.object()["args"].isObject());
        QVERIFY(doc.object()["args"].toObject()["hand"].toString() == "wave");

        reply->deleteLater();
    });

    QObject::connect(reply, &CurlReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestCurlNetworkManager::test_multi()
{
    QThread thread;
    CurlTestMulti async;

    async.moveToThread(&thread);
    async.connect(&thread, SIGNAL(started()), SLOT(start()));
    thread.connect(&async, SIGNAL(finished()), SLOT(quit()));
    QSignalSpy signalFinished(&thread, SIGNAL(finished()));

    thread.start();


    signalFinished.wait(30000);
    thread.wait();
}

void CurlTestMulti::start()
{
    finished_ = 0;
    manager_ = new CurlNetworkManager2(this);

    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 5000, false);
    DnsRequest dnsRequest(NULL, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    for (int i = 0; i < 100; ++i)
    {
        CurlReply *reply = manager_->get(request, dnsRequest.ips());
        connect(reply, SIGNAL(finished()), SLOT(onReplyFinished()));
        replies_ << reply;
    }
    QTimer::singleShot(10, this, SLOT(addMore()));
    QTimer::singleShot(100, this, SLOT(removeOne()));
}

void CurlTestMulti::onReplyFinished()
{
    finished_++;
    CurlReply *reply = qobject_cast<CurlReply *>(sender());
    replies_.remove(reply);
    if (replies_.empty())
    {
        qDebug() << "finished:" << finished_;
        emit finished();
    }
    reply->deleteLater();
}

void CurlTestMulti::removeOne()
{
    if (!replies_.empty())
    {
        (*replies_.begin())->deleteLater();
        replies_.erase(replies_.begin());
    }
    if (replies_.empty())
    {
        qDebug() << "finished:" << finished_;
        emit finished();
    }
    else
    {
        QTimer::singleShot(50, this, SLOT(removeOne()));
    }
}

void CurlTestMulti::addMore()
{
    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 5000, false);
    DnsRequest dnsRequest(NULL, request.url().host());
    dnsRequest.lookupBlocked();
    QVERIFY(dnsRequest.ips().count() > 0);

    for (int i = 0; i < 100; ++i)
    {
        CurlReply *reply = manager_->get(request, dnsRequest.ips());
        connect(reply, SIGNAL(finished()), SLOT(onReplyFinished()));
        replies_ << reply;
    }
    int g = 0;
}

QTEST_MAIN(TestCurlNetworkManager)
