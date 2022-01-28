#include "tst_networkaccessmanager.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>
#include <WinSock2.h>
#include "networkaccessmanager/networkaccessmanager.h"

TestNetworkAccessManager::TestNetworkAccessManager()
{
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
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

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
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

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
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

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
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

    QSignalSpy signalFinished(reply, SIGNAL(finished()));
    signalFinished.wait(20000);
    QCOMPARE(signalFinished.count(), 1);
    QVERIFY(progressCalled > 0);
}
