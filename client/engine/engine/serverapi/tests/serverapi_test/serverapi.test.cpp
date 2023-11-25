#include "serverapi.test.h"
#include "engine/serverapi/requests/loginrequest.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>

ServerApi_test::ServerApi_test() :
    username_("abcracing_free"), password_("lsvlsv123")
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

}

void ServerApi_test::init()
{
    connectStateController_ = new ConnectStateController_moc(this);
    networkDetectionManager_ = new NetworkDetectionManager_moc(this);
    accessManager_ = new NetworkAccessManager(this);
}

void ServerApi_test::cleanup()
{
    delete accessManager_;
    delete networkDetectionManager_;
    delete connectStateController_;
}

void ServerApi_test::testRequests()
{
    QVector<QPair<QString, bool> > failovers;
    failovers << qMakePair("windscribe.com", true);
    failovers << qMakePair("d571d4e00ea4765529703412b1045ba8079d9d87.com", true);
    QScopedPointer<server_api::ServerAPI> serverAPI_(new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                                                              new FailoverContainer_moc(this, failovers)));
    serverAPI_->getHostname();

    {
        server_api::LoginRequest *request = static_cast<server_api::LoginRequest *> (serverAPI_->login(username_, password_, ""));
        connect(request, &server_api::BaseRequest::finished, [request, this]() {
            if (request->networkRetCode() == SERVER_RETURN_SUCCESS)
                authHash_ = request->authHash();
            request->deleteLater();
        });
        QSignalSpy spy(request, &server_api::BaseRequest::finished);
        spy.wait(30000);
    }

    QList<QSharedPointer<QSignalSpy> > spies;
    {
        server_api::BaseRequest *request = serverAPI_->session(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, QStringList());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverCredentials(authHash_, types::Protocol::OPENVPN_TCP);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverConfigs(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->portMap(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->notifications(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->pingTest(5000, true);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->pingTest(5000, false);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->debugLog("aaa_test", "true");
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->recordInstall();
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->confirmEmail(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->webSession(authHash_, WEB_SESSION_PURPOSE_MANAGE_ACCOUNT);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->myIP(5000);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->checkUpdate(UPDATE_CHANNEL_RELEASE);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->speedRating(authHash_, "aaa.com","22.22.22.22", 1);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->staticIps(authHash_, "aaa");
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    {
        server_api::BaseRequest *request = serverAPI_->getRobertFilters(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    {
        server_api::BaseRequest *request = serverAPI_->setRobertFilter(authHash_, types::RobertFilter());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    {
        server_api::BaseRequest *request = serverAPI_->syncRobert(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    serverAPI_->setIgnoreSslErrors(true);

    {
        server_api::BaseRequest *request = serverAPI_->wgConfigsInit(authHash_, "key", false);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    {
        server_api::BaseRequest *request = serverAPI_->wgConfigsConnect(authHash_, "key", "server", "deviceId");
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    serverAPI_->getHostname();

    while (true) {
        bool bAllFinished = true;
        for (const auto &spy : spies) {
            if (spy->count() == 0) {
                bAllFinished = false;
                break;
            }
        }
        if (bAllFinished)
            break;
        QTest::qWait(100);
    }

//    {
//        server_api::BaseRequest *request = static_cast<server_api::BaseRequest *> (serverAPI_->deleteSession(authHash_));
//        connect(request, &server_api::BaseRequest::finished, [request]() {
//            request->deleteLater();
//        });
//        QSignalSpy spy(request, &server_api::BaseRequest::finished);
//        spy.wait(30000);
    //    }
}

void ServerApi_test::testFailoverFailed()
{
    QVector<QPair<QString, bool> > failovers;
    failovers << qMakePair("windscribe.com", false);
    failovers << qMakePair("d571d4e00ea4765529703412b1045ba8079d9d87.com", false);
    QScopedPointer<server_api::ServerAPI> serverAPI_(new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                                                              new FailoverContainer_moc(this, failovers)));
    QList<QSharedPointer<QSignalSpy> > spies;
    {
        server_api::BaseRequest *request = serverAPI_->session(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, QStringList());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverCredentials(authHash_, types::Protocol::OPENVPN_TCP);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }

    while (true) {
        bool bAllFinished = true;
        for (const auto &spy : spies) {
            if (spy->count() == 0) {
                bAllFinished = false;
                break;
            }
        }
        if (bAllFinished)
            break;
        QTest::qWait(100);
    }

    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, QStringList());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        QSignalSpy spy(request, &server_api::BaseRequest::finished);
        spy.wait(30000);
    }
}

void ServerApi_test::testDeleteServerAPIWhileRequestsRunning()
{
    QVector<QPair<QString, bool> > failovers;
    failovers << qMakePair("windscribe.com", true);
    failovers << qMakePair("d571d4e00ea4765529703412b1045ba8079d9d87.com", true);
    QScopedPointer<server_api::ServerAPI> serverAPI_(new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                                                              new FailoverContainer_moc(this, failovers)));
    QList<QSharedPointer<QSignalSpy> > spies;
    {
        server_api::BaseRequest *request = serverAPI_->session(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, QStringList());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
        spies << QSharedPointer<QSignalSpy>(new QSignalSpy(request, &server_api::BaseRequest::finished));
    }
}

void ServerApi_test::testDeleteRequestsBeforeFinished()
{
    QVector<QPair<QString, bool> > failovers;
    failovers << qMakePair("windscribe.com", true);
    failovers << qMakePair("d571d4e00ea4765529703412b1045ba8079d9d87.com", true);
    QScopedPointer<server_api::ServerAPI> serverAPI_(new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                                                              new FailoverContainer_moc(this, failovers)));
    {
        server_api::BaseRequest *request = serverAPI_->session(authHash_);
        request->deleteLater();
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, QStringList());
        request->deleteLater();
    }

    QTest::qWait(10000);
}

QTEST_MAIN(ServerApi_test)
