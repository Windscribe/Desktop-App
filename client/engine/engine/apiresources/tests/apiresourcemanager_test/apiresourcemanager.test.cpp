#include "apiresourcemanager.test.h"
#include <QtTest>
#include <QtConcurrent/QtConcurrent>
#include "utils/utils.h"

#include "engine/failover/failover.h"

ApiResourceManager_test::ApiResourceManager_test() :
    username_("abcracing_free"), password_("lsvlsv123")
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    QCoreApplication::setOrganizationName("Windscribe");
    QCoreApplication::setApplicationName("Windscribe2");
}

void ApiResourceManager_test::init()
{
    connectStateController_ = new ConnectStateController_moc(this);
    networkDetectionManager_ = new NetworkDetectionManager_moc(this);
    accessManager_ = new NetworkAccessManager(this);
    serverAPI_ = new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                           new failover::Failover(nullptr,accessManager_, connectStateController_, "disconnected"),
                                           new failover::Failover(nullptr,accessManager_, connectStateController_, "connected"));

    getMyIPController_ = new GetMyIPController(this, serverAPI_, networkDetectionManager_);
}

void ApiResourceManager_test::cleanup()
{
    delete serverAPI_;
    delete accessManager_;
    delete networkDetectionManager_;
    delete connectStateController_;
}

void ApiResourceManager_test::basicTest()
{
    api_resources::ApiResourcesManager *resourcesManager = new api_resources::ApiResourcesManager(this, serverAPI_, connectStateController_);

    connect(getMyIPController_, &GetMyIPController::answerMyIP, [this]() {
        qDebug() << "my Ip";
        //getMyIPController_->getIPFromDisconnectedState(1);
    });

    for (int i = 0; i < 100; ++i) {
        QMetaObject::invokeMethod(this, [this]() {
            getMyIPController_->getIPFromDisconnectedState(Utils::generateIntegerRandom(0, 150));
        });
    }

    //connect(resourcesManager, &api_resources::ApiResourcesManager::readyForLogin, [this]() {
    getMyIPController_->getIPFromDisconnectedState(1);
    //});

    resourcesManager->fetchAllWithAuthHash();

    QTest::qWait(60000);
}

QTEST_MAIN(ApiResourceManager_test)
