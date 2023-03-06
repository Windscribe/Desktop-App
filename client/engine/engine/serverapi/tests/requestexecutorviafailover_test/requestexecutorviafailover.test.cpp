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
    failover::BaseFailover *failover = new failover::EchFailover(this, "3e60e3d5-d379-46cc-a9a0-d9f04f47999a", networkAccessManager_, "https://1.1.1.1/dns-query", "echconfig001.windscribe.dev", "ech-public-test.windscribe.dev", connectStateController_);

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

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
    failover::BaseFailover *failover = new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, HardcodedSettings::instance().dynamicDomainsUrls().at(0), HardcodedSettings::instance().dynamicDomains().at(0));

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

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
    failover::BaseFailover *failover = new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, "bad_domain_32764623746873246.com", HardcodedSettings::instance().dynamicDomains().at(0));

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

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
    failover::BaseFailover *failover = new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, HardcodedSettings::instance().dynamicDomainsUrls().at(0), HardcodedSettings::instance().dynamicDomains().at(0));

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

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
    failover::BaseFailover *failover = new failover::DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, HardcodedSettings::instance().dynamicDomainsUrls().at(0), HardcodedSettings::instance().dynamicDomains().at(0));

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

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
    failover::BaseFailover *failover = new failover::AccessIpsFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager_, HardcodedSettings::instance().apiIps()[0]);

    QSignalSpy spy(requestExecuter, SIGNAL(finished(server_api::RequestExecuterRetCode)));

    requestExecuter->execute(request, failover, false);
    while (spy.count() != 1) {
        spy.wait(1000);
    }

    QList<QVariant> arguments = spy.takeFirst();
    qDebug() << failover->name() << arguments.at(0).value<server_api::RequestExecuterRetCode>() << request->networkRetCode() << request->ip();
}

QTEST_MAIN(RequestExecuterViaFailover_test)
