#include "networkaccessmanager.test.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>
#ifdef Q_OS_WIN
#include <WinSock2.h>
#endif
#include "engine/networkaccessmanager/networkaccessmanager.h"

TestNetworkAccessManager::TestNetworkAccessManager()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

TestNetworkAccessManager::~TestNetworkAccessManager()
{
}

void TestNetworkAccessManager::test_get()
{
    NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 20000, false);
    NetworkAccessManager *manager = new NetworkAccessManager(this);
    NetworkReply *reply = manager->get(request);
    int progressCalled = 0;
    QObject::connect(reply, &NetworkReply::finished, this, [=]
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


    QObject::connect(reply, &NetworkReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, &NetworkReply::finished);
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestNetworkAccessManager::test_post()
{
    NetworkRequest request(QUrl("https://postman-echo.com/post?hand=wave"), 20000, false);
    NetworkAccessManager *manager = new NetworkAccessManager(this);
    NetworkReply *reply = manager->post(request, "postdata=testtest");
    int progressCalled = 0;
    QObject::connect(reply, &NetworkReply::finished, this, [=]
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


    QObject::connect(reply, &NetworkReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, &NetworkReply::finished);
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestNetworkAccessManager::test_put()
{
    NetworkRequest request(QUrl("https://postman-echo.com/put?hand=wave"), 20000, false);
    NetworkAccessManager *manager = new NetworkAccessManager(this);
    NetworkReply *reply = manager->put(request, "putdata=testtest");
    int progressCalled = 0;
    QObject::connect(reply, &NetworkReply::finished, this, [=]
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

    QObject::connect(reply, &NetworkReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, &NetworkReply::finished);
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestNetworkAccessManager::test_delete()
{
    NetworkRequest request(QUrl("https://postman-echo.com/delete?hand=wave"), 20000, false);
    NetworkAccessManager *manager = new NetworkAccessManager(this);
    NetworkReply *reply = manager->deleteResource(request);
    int progressCalled = 0;
    QObject::connect(reply, &NetworkReply::finished, this, [=]
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

    QObject::connect(reply, &NetworkReply::progress, this, [&progressCalled]
    {
        progressCalled++;
    });

    QSignalSpy signalFinished(reply, &NetworkReply::finished);
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}

void TestNetworkAccessManager::test_whitelist()
{
    {
        NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 20000, false);
        NetworkAccessManager *manager = new NetworkAccessManager(this);
        NetworkReply *reply = manager->get(request);
        QObject::connect(reply, &NetworkReply::finished, this, [=] {
            reply->deleteLater();
        });

        QSignalSpy signalWhitelist(manager, &NetworkAccessManager::whitelistIpsChanged);

        QSignalSpy signalFinished(reply, &NetworkReply::finished);
        signalFinished.wait(20000);

        QCOMPARE(signalWhitelist.count(), 1);
        QList<QVariant> arguments = signalWhitelist.takeFirst();
        QSet<QString> set = qvariant_cast<QSet<QString> >(arguments.at(0));
        QVERIFY(set.size() > 0);
    }

    {
        NetworkRequest request(QUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2"), 20000, false);
        request.setRemoveFromWhitelistIpsAfterFinish();
        NetworkAccessManager *manager = new NetworkAccessManager(this);
        NetworkReply *reply = manager->get(request);
        QObject::connect(reply, &NetworkReply::finished, this, [=] {
            reply->deleteLater();
        });

        QSignalSpy signalWhitelist(manager, &NetworkAccessManager::whitelistIpsChanged);

        QSignalSpy signalFinished(reply, &NetworkReply::finished);
        signalFinished.wait(20000);

        QCOMPARE(signalWhitelist.count(), 2);
        QList<QVariant> arguments = signalWhitelist.at(0);
        QSet<QString> set = qvariant_cast<QSet<QString> >(arguments.at(0));
        QVERIFY(set.size() > 0);

        arguments = signalWhitelist.at(1);
        set = qvariant_cast<QSet<QString> >(arguments.at(0));
        QVERIFY(set.size() == 0);
    }
}

QTEST_MAIN(TestNetworkAccessManager)
