#include "requestexecutorviafailover.test.h"

#include <QtTest>
#include <QtConcurrent/QtConcurrent>

#include "engine/serverapi/requestexecuterviafailover.h"
#include "engine/serverapi/requests/myiprequest.h"
#include "engine/failover/failovers/dynamicdomainfailover.h"
#include "engine/failover/failovers/accessipsfailover.h"
#include "engine/failover/failovers/echfailover.h"
#include "utils/hardcodedsettings.h"

RequestExecuterViaFailover_test::RequestExecuterViaFailover_test()
{
#ifdef Q_OS_WIN
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

void RequestExecuterViaFailover_test::init()
{
    bool bDgaLoaded = dga_.load();
    QVERIFY(bDgaLoaded);
    networkAccessManager_ = new NetworkAccessManager(this);
    connectStateController_ = new ConnectStateController_moc(this);
}

void RequestExecuterViaFailover_test::cleanup()
{
    delete networkAccessManager_;
    delete connectStateController_;
}

void RequestExecuterViaFailover_test::testEchFailover()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::EchFailover(this, "3e60e3d5-d379-46cc-a9a0-d9f04f47999a", networkAccessManager_,
                                                                                                                       dga_.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga_.getParameter(PAR_ECH_CONFIG_DOMAIN), dga_.getParameter(PAR_ECH_DOMAIN), false));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>() << request->networkRetCode() << request->ip() << requestExecuter->failoverData();
}

void RequestExecuterViaFailover_test::testDynamicFailover()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, dga_.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga_.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>() << request->networkRetCode() << request->ip() << requestExecuter->failoverData();
}

void RequestExecuterViaFailover_test::testFailoverFailed()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, "bad_domain_32764623746873246.com", dga_.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>() << request->networkRetCode() << request->ip();
}

void RequestExecuterViaFailover_test::testRequestDeleted()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, dga_.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga_.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        if (request) {
            delete request;
            request = nullptr;
        }
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>();
}

void RequestExecuterViaFailover_test::testConnectStateChanged()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, dga_.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga_.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        connectStateController_->setState(CONNECT_STATE_CONNECTED);
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>();
    connectStateController_->setState(CONNECT_STATE_DISCONNECTED);
}

void RequestExecuterViaFailover_test::testMultiIpFailover()
{
    server_api::RequestExecuterViaFailover *requestExecuter = new server_api::RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_);
    server_api::MyIpRequest *request = new server_api::MyIpRequest(this, 5000);
    QSharedPointer<failover::BaseFailover> failover = QSharedPointer<failover::BaseFailover>(new failover::AccessIpsFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, dga_.getParameter(PAR_API_ACCESS_IP1)));

    QSignalSpy spy(requestExecuter, &server_api::RequestExecuterViaFailover::finished);

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>() << request->networkRetCode() << request->ip();
}

QTEST_MAIN(RequestExecuterViaFailover_test)
