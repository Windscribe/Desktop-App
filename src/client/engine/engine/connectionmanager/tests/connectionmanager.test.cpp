#include <QSettings>
#include <QSignalSpy>
#include <QtTest>

#include "connectionmanager.test.h"

#include <memory>
#include <variant>

#include "api_responses/portmap.h"
#include "engine/connectionmanager/connectionmanager.h"
#include "engine/connectionmanager/isleepevents.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include "engine/dns/dnsconfigurator.h"
#include "engine/helper/helper.h"
#include "fakes.h"
#include "types/connecteddnsinfo.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/proxysettings.h"
#include "utils/log/logger.h"

void TestConnectionManager::initTestCase()
{
    QCoreApplication::setOrganizationName("WindscribeTest");
    QCoreApplication::setApplicationName("connectionmanager.test");

    // CM wires the connector signals with Qt::QueuedConnection; make sure the argument types can be
    // marshalled and captured by QSignalSpy even though engine init did not run. AdapterGatewayInfo
    // registers itself at static-init in adaptergatewayinfo.cpp.
    qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
    qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");
}

void TestConnectionManager::cleanupTestCase()
{
    QSettings settings;
    settings.clear();
}

void TestConnectionManager::init()
{
    helper_ = new Helper(std::unique_ptr<IHelperBackend>(new FakeHelperBackend()),
                         log_utils::Logger::instance().getSpdLogger("basic"));
    networkDetectionManager_ = new FakeNetworkDetectionManager();
    credentialsStorage_ = new CustomOvpnAuthCredentialsStorage();
    connectionFactory_ = new FakeConnectionFactory();
    platformPolicy_ = new FakePlatformPolicy();
    connSettingsPolicyFactory_ = new FakeConnSettingsPolicyFactory();
    sleepEvents_ = new FakeSleepEvents();
    extraConfig_ = new FakeExtraConfig();
    dnsPlatformPolicy_ = new FakePlatformPolicy();
    ctrldManager_ = new FakeCtrldManager();
    dnsConfigurator_ = new DnsConfigurator(nullptr, helper_, dnsPlatformPolicy_, ctrldManager_);

    // CM takes ownership of the seam objects; the DnsConfigurator stays caller-owned, as in Engine.
    cm_ = new ConnectionManager(nullptr, helper_, networkDetectionManager_, credentialsStorage_, dnsConfigurator_,
                                connectionFactory_, platformPolicy_, connSettingsPolicyFactory_, sleepEvents_,
                                extraConfig_);

    // Default: an automatic-mode IKEv2 default node. IKEv2 is the simplest happy-path protocol because
    // it needs no external process, ovpn file generation, or WireGuard config fetch.
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeIkev2Descr());
    connSettingsPolicyFactory_->setAutomaticMode(true);
}

void TestConnectionManager::cleanup()
{
    delete cm_;
    cm_ = nullptr;
    // The seam objects are owned by cm_ and already gone. The DnsConfigurator owns its policy and
    // ctrld manager.
    delete dnsConfigurator_;
    delete credentialsStorage_;
    delete networkDetectionManager_;
    delete helper_;
    dnsConfigurator_ = nullptr;
    dnsPlatformPolicy_ = nullptr;
    ctrldManager_ = nullptr;
    credentialsStorage_ = nullptr;
    networkDetectionManager_ = nullptr;
    helper_ = nullptr;
}

AdapterGatewayInfo TestConnectionManager::vpnAdapterInfo()
{
    AdapterGatewayInfo info;
    info.setAdapterName("tun0");
    info.addGatewayIp(types::IpAddress("10.0.0.1"));
    return info;
}

CurrentConnectionDescr TestConnectionManager::makeIkev2Descr()
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_DEFAULT;
    d.protocol = types::Protocol::IKEV2;
    d.ip = "10.0.0.2";
    d.hostname = "ikev2.example.com";
    d.port = 500;
    d.isIpv6Support = false;
    return d;
}

CurrentConnectionDescr TestConnectionManager::makeWireGuardDescr()
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_DEFAULT;
    d.protocol = types::Protocol::WIREGUARD;
    d.ip = "10.0.0.3";
    d.hostname = "wireguard.example.com";
    d.port = 51820;
    d.isIpv6Support = false;
    return d;
}

CurrentConnectionDescr TestConnectionManager::makeStaticIpIkev2Descr()
{
    CurrentConnectionDescr d = makeIkev2Descr();
    d.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
    return d;
}

types::ConnectedDnsInfo TestConnectionManager::makeDnsInfo(CONNECTED_DNS_TYPE type, const QString &upstream)
{
    types::ConnectedDnsInfo dnsInfo;
    dnsInfo.type = type;
    dnsInfo.upStream1 = upstream;
    return dnsInfo;
}

types::ConnectedDnsInfo TestConnectionManager::makeControldDnsInfo()
{
    return makeDnsInfo(CONNECTED_DNS_TYPE_CONTROLD, "https://dns.controld.com/abcd1234");
}

FakeConnection *TestConnectionManager::startConnectingIkev2()
{
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    return connectionFactory_->lastCreated();
}

FakeConnection *TestConnectionManager::connectIkev2()
{
    return connectIkev2With(vpnAdapterInfo());
}

FakeConnection *TestConnectionManager::connectIkev2With(const AdapterGatewayInfo &info)
{
    FakeConnection *conn = startConnectingIkev2();
    conn->driveConnected(info);
    // A timeout here surfaces through the callers' state assertions.
    (void)QTest::qWaitFor([this] { return cm_->state_ == cm_->STATE_CONNECTED; });
    return conn;
}

FakeConnection *TestConnectionManager::connectWireGuard()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    // Stage 4 (uniformly async prepare): the dial happens one event-loop hop after prepared().
    FakeConnection *conn = connectionFactory_->lastCreated();
    // A timeout here surfaces through the callers' startConnectCount assertions.
    (void)QTest::qWaitFor([conn] { return conn->startConnectCount() > 0; });
    return conn;
}

FakeConnection *TestConnectionManager::startConnectingWireGuardAtKeyLimit()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitUserInputRequired);
    connectionFactory_->setPrepareInputType(UserInputType::KeyLimitConsent);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    return connectionFactory_->lastCreated();
}

void TestConnectionManager::testConstructAndDestructWithFakes()
{
    QVERIFY(cm_->isDisconnected());
    QVERIFY(!cm_->timerReconnection_.isActive());
    QVERIFY(!cm_->connectTimer_.isActive());
    QVERIFY(!cm_->connectingTimer_.isActive());
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
}

void TestConnectionManager::testHappyPathConnect()
{
    QSignalSpy hostnameSpy(cm_, &ConnectionManager::connectingToHostname);
    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);

    FakeConnection *conn = connectIkev2();
    QVERIFY(conn);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(hostnameSpy.count(), 1);
    QCOMPARE(conn->startConnectCount(), 1);
    QVERIFY(!cm_->connectingTimer_.isActive());
    // The connect path applies its platform tweaks: DoH disabled for auto-DNS and IPv4 gai priority
    // enabled for the (non-IPv6) IKEv2 tunnel.
    QVERIFY(dnsPlatformPolicy_->disableDohCount() >= 1);
    QVERIFY(platformPolicy_->lastGaiIpv4Priority());
}

void TestConnectionManager::testHappyPathConnectStaticIpNode()
{
    // A static-IP node takes the same connect path as a default node (both handled by the
    // DEFAULT/STATIC_IPS branch of doConnectPart). Pin that it reaches CONNECTED and emits connected().
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeStaticIpIkev2Descr());

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    FakeConnection *conn = connectIkev2();
    QVERIFY(conn);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(conn->startConnectCount(), 1);
}

void TestConnectionManager::testTunnelTestPassAfterConnect()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // Pretend an earlier cached-WG failover consumed part of the budget.
    cm_->wgCachedConfigAttempts_ = 3;

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestsFinished(true, "203.0.113.7");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), true);
    QCOMPARE(testTunnelSpy.at(0).at(1).toString(), QString("203.0.113.7"));
    // Automatic mode records the successful attempt so a later failover won't immediately give up.
    QVERIFY(cm_->bWasSuccessfullyConnectionAttempt_);
    // A working tunnel refreshes the cached-WG budget so subsequent reconnects can retry it.
    QCOMPARE(cm_->wgCachedConfigAttempts_, 0);
}

void TestConnectionManager::testUserDisconnectFromConnected()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    // The connection policy is reset on a user disconnect.
    QVERIFY(connSettingsPolicyFactory_->lastCreated()->resetCount() >= 1);
}

void TestConnectionManager::testUserDisconnectFromConnecting()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTING_FROM_USER_CLICK);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testUserDisconnectWhenNoConnector()
{
    // No connect attempt: clickDisconnect from the disconnected state with no connector emits
    // disconnected() synchronously.
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect(DISCONNECTED_BY_USER);

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testFailoverRetriesWhenNotExhausted()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveError(CONNECT_ERROR::TCP_ERROR);

    // Not exhausted -> reconnect: reconnecting() emitted and a fresh connector is created.
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testFailoverExhaustedEmitsDisconnectedItself()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    connSettingsPolicyFactory_->lastCreated()->setFailed(true);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn->driveError(CONNECT_ERROR::TCP_ERROR);

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    // The failed attempt was recorded on the policy before giving up.
    QVERIFY(connSettingsPolicyFactory_->lastCreated()->putFailedConnectionCount() >= 1);
}

void TestConnectionManager::testErrorClassificationAuthErrorFatalWhenEmit()
{
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    req.bEmitAuthError = true;
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);

    // With bEmitAuthError the auth error is fatal but is surfaced only once the connector confirms
    // it stopped.
    conn->driveError(CONNECT_ERROR::AUTH_ERROR);
    QTRY_COMPARE(cm_->state_, cm_->STATE_ERROR_DURING_CONNECTION);
    QCOMPARE(errorSpy.count(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::AUTH_ERROR);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testErrorClassificationImmediateStop()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    conn->driveError(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);

    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testAuthErrorRetryableWhenNotEmit()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Without bEmitAuthError the auth error is retryable; CM waits for the connector's own
    // disconnected() rather than forcing a disconnect.
    conn->driveError(CONNECT_ERROR::AUTH_ERROR);

    QTRY_COMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testManualModeWireGuardErrorIsFatal()
{
    // Classification depends on error code + mode, not on the live connector's protocol, so an IKEv2
    // connector in manual mode faithfully exercises the WireGuard-error branch.
    connSettingsPolicyFactory_->setAutomaticMode(false);
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // Fatal: CM parks in the error state and surfaces errorDuringConnection only once the connector
    // confirms it stopped (same deferral as the fatal auth-error path).
    conn->driveError(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
    QTRY_COMPARE(cm_->state_, cm_->STATE_ERROR_DURING_CONNECTION);
    QCOMPARE(errorSpy.count(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testAutomaticModeWireGuardErrorIsRetryable()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveError(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QVERIFY(cm_->state_ != cm_->STATE_ERROR_DURING_CONNECTION);
}

void TestConnectionManager::testWireGuardErrorClearsCachedWgConfigWhenExhausted()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    cm_->hasUsableCachedWgConfig_ = true;
    cm_->wgCachedConfigAttempts_ = cm_->kMaxWgCachedConfigAttempts;
    QSettings settings;
    settings.setValue("wireguardConfig", "stale");

    conn->driveError(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);

    QTRY_VERIFY(!cm_->hasUsableCachedWgConfig_);
    QVERIFY(!settings.contains("wireguardConfig"));
}

void TestConnectionManager::testReconnectionTimerExpiry()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // Simulate the 1-hour reconnection cap firing.
    cm_->onTimerReconnection();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testConnectingTimeout()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // The connecting-timeout slot behaves as if the single-shot timer expired.
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testConnectingTimeoutClearsCachedWgConfigWhenExhausted()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    // A cached config with stale credentials starts and configures fine but never completes the
    // handshake, so the failure surfaces as a connecting timeout, not a connector error.
    cm_->currentConnectionDescr_.protocol = types::Protocol::WIREGUARD;
    cm_->hasUsableCachedWgConfig_ = true;
    cm_->wgCachedConfigAttempts_ = cm_->kMaxWgCachedConfigAttempts;
    QSettings settings;
    settings.setValue("wireguardConfig", "stale");

    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QVERIFY(!cm_->hasUsableCachedWgConfig_);
    QVERIFY(!settings.contains("wireguardConfig"));
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testConnectingTimeoutKeepsCachedWgConfig()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();
    // Below the attempt cap a timeout may be transient; the config must survive it.
    cm_->currentConnectionDescr_.protocol = types::Protocol::WIREGUARD;
    cm_->hasUsableCachedWgConfig_ = true;
    cm_->wgCachedConfigAttempts_ = cm_->kMaxWgCachedConfigAttempts - 1;
    QSettings settings;
    settings.setValue("wireguardConfig", "stale");

    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QVERIFY(cm_->hasUsableCachedWgConfig_);
    QVERIFY(settings.contains("wireguardConfig"));

    // A non-WireGuard attempt timing out later in the rotation must not discard the config either,
    // even with the budget exhausted.
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    cm_->currentConnectionDescr_.protocol = types::Protocol(types::Protocol::OPENVPN_UDP);
    cm_->wgCachedConfigAttempts_ = cm_->kMaxWgCachedConfigAttempts;
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QVERIFY(cm_->hasUsableCachedWgConfig_);
    QVERIFY(settings.contains("wireguardConfig"));
    settings.remove("wireguardConfig");
}

void TestConnectionManager::testNetworkOfflineWhileConnected()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // Keep the connector alive so the post-offline state is observable deterministically.
    conn->setAutoEmitDisconnected(false);

    QSignalSpy connectivitySpy(cm_, &ConnectionManager::internetConnectivityChanged);
    networkDetectionManager_->setOnline(false);

    QCOMPARE(connectivitySpy.count(), 1);
    QCOMPARE(connectivitySpy.at(0).at(0).toBool(), false);

#ifdef Q_OS_MACOS
    // Only macOS drives the connection state machine off network-online changes.
    QCOMPARE(cm_->state_, cm_->STATE_WAIT_FOR_NETWORK_CONNECTIVITY);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), 1);
#else
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
#endif
}

void TestConnectionManager::testSleepWakeReconnect()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    sleepEvents_->driveSleep();

    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_SLEEP_MODE_NEED_RECONNECT);

    const int createdBefore = connectionFactory_->createdCount();
    sleepEvents_->driveWake();

    // Wake restores the connection: the connector is torn down and a fresh attempt begins.
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testWakeDuringBlockingDisconnect()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    // Model a wake event queued while blockingDisconnect() is still spinning. On non-Windows the loop
    // pumps the event loop, so onWakeMode() runs mid-loop and reconnects via restoreConnectionAfterWakeUp().
    // On Windows the loop deliberately pumps nothing: waitForDisconnect() blocks until the stop completes,
    // and the queued wake is only delivered after onSleepMode() returns, reconnecting from
    // STATE_SLEEP_MODE_NEED_RECONNECT. Either route lands in STATE_WAKEUP_RECONNECTING rather than parking
    // in the sleep state.
    conn->setAutoEmitDisconnected(false);
    QTimer::singleShot(0, sleepEvents_, [this]() { sleepEvents_->driveWake(); });
#ifndef Q_OS_WIN
    QTimer::singleShot(0, conn, [conn]() { conn->driveDisconnected(); });
#endif

    sleepEvents_->driveSleep();

    QTRY_COMPARE(cm_->state_, cm_->STATE_WAKEUP_RECONNECTING);
}

void TestConnectionManager::testWireGuardKeyLimitPrompt()
{
    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    startConnectingWireGuardAtKeyLimit();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTING_FROM_USER_CLICK);

    QTRY_COMPARE(keyLimitSpy.count(), 1);
    // Timeout is paused while awaiting the user's key-limit decision.
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testWireGuardKeyLimitDecline()
{
    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    startConnectingWireGuardAtKeyLimit();
    QTRY_COMPARE(keyLimitSpy.count(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->onWireGuardKeyLimitUserResponse(false);

    // Stage 4 (early creation): a live connector exists here, so the disconnect completes
    // asynchronously instead of through the old null-connector synchronous branch.
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testWireGuardKeyLimitConsentResumesConnector()
{
    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    FakeConnection *conn = startConnectingWireGuardAtKeyLimit();
    QTRY_COMPARE(keyLimitSpy.count(), 1);
    QVERIFY(!cm_->connectingTimer_.isActive());

    cm_->onWireGuardKeyLimitUserResponse(true);

    // The consent goes to the connector, which resumes its own paused fetch; the connecting
    // timeout resumes with the answer.
    QCOMPARE(conn->continueWithUserInputCount(), 1);
    QVERIFY(std::get_if<KeyLimitConsentResponse>(&conn->lastUserInput()) != nullptr);
    QVERIFY(cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testStaleConnectorSignalsIgnored()
{
    // error()/reconnecting() from a retired connector, queued behind its disconnected(), must not
    // act on the manager (they would count failures / stop the DNS proxy against a later attempt).
    FakeConnection *conn = startConnectingIkev2();
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveDisconnected();
    conn->driveError(CONNECT_ERROR::TCP_ERROR);
    conn->driveReconnecting();

    QTRY_COMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QTest::qWait(10);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(connSettingsPolicyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testSleepDuringPrepareQuiescesAttempt()
{
    // Sleep landing mid-prepare tears the pre-dial attempt down (wrappers/fetches quiesced) and
    // wake restores it from scratch.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnectingIkev2();
    QCOMPARE(conn->startConnectCount(), 0);

    sleepEvents_->driveSleep();

    QCOMPARE(conn->teardownCount(), 1);
    QCOMPARE(cm_->state_, cm_->STATE_SLEEP_MODE_NEED_RECONNECT);

    sleepEvents_->driveWake();
    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
}

void TestConnectionManager::testWireGuardKeyLimitIgnoredDuringDisconnect()
{
    // A key-limit request already queued when the user starts disconnecting must not raise the prompt.
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    conn->setAutoEmitDisconnected(false);

    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    cm_->clickDisconnect();
    conn->driveUserInputRequired(UserInputType::KeyLimitConsent);
    QTest::qWait(10);
    QCOMPARE(keyLimitSpy.count(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QCOMPARE(keyLimitSpy.count(), 0);
}

void TestConnectionManager::testWireGuardKeyLimitAnswerIgnoredWhenDisconnected()
{
    // A late answer (e.g. the GUI prompt resolved after the attempt ended) must not re-drive a
    // disconnect or arm the connecting timer.
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);

    cm_->onWireGuardKeyLimitUserResponse(false);
    QTest::qWait(10);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);

    cm_->onWireGuardKeyLimitUserResponse(true);
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testRetryPolicyWalksOnBareDisconnect()
{
    // An endpoint-list policy consumes the endpoint and dials the next one when the process dies
    // without a classified error.
    connSettingsPolicyFactory_->setRetryOnAttemptFailure(true);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnectingIkev2();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveDisconnected();

    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(connSettingsPolicyFactory_->lastCreated()->putFailedConnectionCount(), 1);
}

void TestConnectionManager::testRetryPolicyBareDisconnectExhaustedStops()
{
    connSettingsPolicyFactory_->setRetryOnAttemptFailure(true);
    connSettingsPolicyFactory_->setFailed(true);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnectingIkev2();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testRetryPolicyWalksOnFatalError()
{
    // Local fatal errors (spawn/socket/adapter) walk the endpoint list instead of ending the attempt.
    connSettingsPolicyFactory_->setRetryOnAttemptFailure(true);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    FakeConnection *conn = startConnectingIkev2();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveError(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);

    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(errorSpy.count(), 0);
}

void TestConnectionManager::testBlockingDisconnectQuiescesPreDialAttempt()
{
    // A pre-dial attempt (prepare in flight) is fully cancelled by blockingDisconnect: teardown runs
    // and the manager ends up disconnected without emitting.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnectingIkev2();
    QCOMPARE(conn->startConnectCount(), 0);

    cm_->blockingDisconnect(false);

    QCOMPARE(conn->teardownCount(), 1);
    QVERIFY(cm_->isDisconnected());
    QCOMPARE(disconnectedSpy.count(), 0);
}

void TestConnectionManager::testWireGuardConfigFetchFailedManual()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutomaticMode(false);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    cm_->clickConnect(req);

    // Manual mode has no next option to advance to, so the API-exhausted failure is fatal.
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testWireGuardConfigFetchFailedAutomatic()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutomaticMode(true);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Automatic mode retries (lets the policy advance) rather than surfacing a fatal error.
    cm_->clickConnect(req);

    QTRY_VERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testConfigFetchFailedFailsOverInManualMode()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutomaticMode(false);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(CONNECT_ERROR::CONFIG_FETCH_FAILED);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    cm_->clickConnect(req);

    // Unlike the API-exhausted code, a plain fetch failure routes to failover in BOTH modes.
    QTRY_VERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testUpdateConnectionSettingsWhileConnectedReconnects()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testContinueWithUsernamePasswordForwards()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);

    cm_->continueWithUsernameAndPassword("user", "pass", false);
    QCOMPARE(conn->continueWithUserInputCount(), 1);
    const auto *response = std::get_if<UsernameResponse>(&conn->lastUserInput());
    QVERIFY(response != nullptr);
    QCOMPARE(response->username, QString("user"));
    QCOMPARE(response->password, QString("pass"));
}

void TestConnectionManager::testContinueWithUsernamePasswordReconnect()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    cm_->continueWithUsernameAndPassword("user", "pass", true);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTING_FROM_USER_CLICK);
    QVERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testSpontaneousDropFromConnectedReconnects()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Connector drops on its own (not via an error) while connected -> CM reconnects.
    conn->driveDisconnected();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
    QVERIFY(cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testReconnectPublicMethod()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->reconnect();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testTunnelTestFailedRetriesWhenNotExhausted()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->onTunnelTestsFinished(false, "");

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testTunnelTestFailedExhaustedDisconnects()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    connSettingsPolicyFactory_->lastCreated()->setFailed(true);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->onTunnelTestsFinished(false, "");

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testTunnelTestFailedCustomConfigEmitsResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // Custom configs surface the tunnel-test failure to the UI instead of failing over.
    connSettingsPolicyFactory_->lastCreated()->setCustomConfig(true);

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestsFinished(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
}

void TestConnectionManager::testTunnelTestNoErrorFlagSurfacesResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // The ExtraConfig no-error flag surfaces the failure to the UI without failing over, regardless
    // of mode or custom-config. Drive it through the tunnel-test seam so the branch is pinned
    // rather than depending on an on-disk config.
    extraConfig_->setTunnelTestNoError(true);

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestsFinished(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
}

void TestConnectionManager::testProtocolChangedStartsRestTimer()
{
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    // A protocol change inserts a 10s rest between failover attempts rather than reconnecting at once.
    connSettingsPolicyFactory_->lastCreated()->setProtocolChanged(true);
    const int createdBefore = connectionFactory_->createdCount();

    conn->driveError(CONNECT_ERROR::TCP_ERROR);

    QTRY_VERIFY(cm_->connectTimer_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    // The rest timer expiring triggers the next attempt.
    cm_->connectTimer_.stop();
    cm_->onConnectTrigger();
    QVERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testWakeWhileOfflineWaitsForNetwork()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    // Waking while the network is still down parks in the wait-for-connectivity state.
    cm_->bLastIsOnline_ = false;
    sleepEvents_->driveWake();

    QCOMPARE(cm_->state_, cm_->STATE_WAIT_FOR_NETWORK_CONNECTIVITY);
}

void TestConnectionManager::testConnectorReconnectingSignalFromConnected()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // Keep the connector alive so the reconnecting transition is observable before teardown.
    conn->setAutoEmitDisconnected(false);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // The connector itself reports it is reconnecting (e.g. OpenVPN mid-session) -> CM moves to
    // RECONNECTING, arms the reconnection cap, and tells the connector to stop.
    conn->driveReconnecting();

    QTRY_COMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), 1);
}

void TestConnectionManager::testOnlineButNoDefaultAdapterWaitsForNetwork()
{
    // Online, but the OS reports no usable default adapter yet: doConnect parks and polls for
    // connectivity instead of attempting the connection.
    platformPolicy_->setDefaultAdapter(AdapterGatewayInfo());
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QVERIFY(cm_->timerReconnection_.isActive());
    QVERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), 0);

    // Once a default adapter appears, the poll tick proceeds with the connection.
    platformPolicy_->setDefaultAdapter(vpnAdapterInfo());
    cm_->onTimerWaitNetworkConnectivity();

    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
    QVERIFY(connectionFactory_->createdCount() > 0);
}

void TestConnectionManager::testNoWaitPolicyFailsFastWhenOffline()
{
    // A policy that opts out of waiting for connectivity (emergency connect) surfaces the failure
    // immediately instead of parking in the wait-for-network poll.
    connSettingsPolicyFactory_->setWaitForNetwork(false);
    networkDetectionManager_->setOnline(false);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), 0);
}

void TestConnectionManager::testLockdownModeIkev2Error()
{
    // Manual-mode IKEv2 while lockdown mode blocks IKEv2: doConnect surfaces a fatal error and never
    // creates a connector.
    connSettingsPolicyFactory_->setAutomaticMode(false);
    platformPolicy_->setLockdownBlockingIkev2(true);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::LOCKDOWN_MODE_IKEV2);
    QCOMPARE(connectionFactory_->createdCount(), 0);
}

void TestConnectionManager::testTunnelTestAttemptsZeroSurfacesResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    // ExtraConfig pins the tunnel-test attempt count to 0: the result is surfaced to the UI as-is
    // without failing over. Drive it through the tunnel-test seam rather than an on-disk config.
    extraConfig_->setTunnelTestAttempts(true, 0);

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestsFinished(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
}

void TestConnectionManager::testRequestUsernameForwarded()
{
    // The connector asking for a username is forwarded to the UI carrying the custom-config path
    // taken from the current connection descriptor.
    CurrentConnectionDescr d = makeIkev2Descr();
    d.customConfigFilename = "/configs/my.ovpn";
    connSettingsPolicyFactory_->setCurrentConnectionSettings(d);
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);

    QSignalSpy usernameSpy(cm_, &ConnectionManager::requestUsername);
    conn->driveUserInputRequired(UserInputType::Username);

    QTRY_COMPARE(usernameSpy.count(), 1);
    QCOMPARE(usernameSpy.at(0).at(0).toString(), QString("/configs/my.ovpn"));
}

void TestConnectionManager::testRequestPasswordForwarded()
{
    CurrentConnectionDescr d = makeIkev2Descr();
    d.customConfigFilename = "/configs/my.ovpn";
    connSettingsPolicyFactory_->setCurrentConnectionSettings(d);
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);

    QSignalSpy passwordSpy(cm_, &ConnectionManager::requestPassword);
    conn->driveUserInputRequired(UserInputType::Password);

    QTRY_COMPARE(passwordSpy.count(), 1);
    QCOMPARE(passwordSpy.at(0).at(0).toString(), QString("/configs/my.ovpn"));
}

void TestConnectionManager::testUsernameRequestAnsweredFromSessionCache()
{
    // Credentials the user supplied for a reconnect are cached for the session; when the fresh
    // connector asks for a username, the relay answers it directly instead of re-prompting the UI.
    startConnectingIkev2();
    cm_->continueWithUsernameAndPassword("user1", "pass1", true);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);

    QSignalSpy usernameSpy(cm_, &ConnectionManager::requestUsername);
    conn->driveUserInputRequired(UserInputType::Username);

    QTRY_COMPARE(conn->continueWithUserInputCount(), 1);
    const auto *response = std::get_if<UsernameResponse>(&conn->lastUserInput());
    QVERIFY(response);
    QCOMPARE(response->username, QString("user1"));
    QCOMPARE(response->password, QString("pass1"));
    QCOMPARE(usernameSpy.count(), 0);
}

void TestConnectionManager::testPasswordRequestAnsweredFromSessionCache()
{
    startConnectingIkev2();
    cm_->continueWithUsernameAndPassword("user1", "pass1", true);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);

    QSignalSpy passwordSpy(cm_, &ConnectionManager::requestPassword);
    conn->driveUserInputRequired(UserInputType::Password);

    QTRY_COMPARE(conn->continueWithUserInputCount(), 1);
    const auto *response = std::get_if<PasswordResponse>(&conn->lastUserInput());
    QVERIFY(response);
    QCOMPARE(response->password, QString("pass1"));
    QCOMPARE(passwordSpy.count(), 0);
}

void TestConnectionManager::testStatisticsUpdatedForwarded()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy statsSpy(cm_, &ConnectionManager::statisticsUpdated);
    conn->driveStatisticsUpdated(111, 222, true);

    QTRY_COMPARE(statsSpy.count(), 1);
    QCOMPARE(statsSpy.at(0).at(0).toULongLong(), quint64(111));
    QCOMPARE(statsSpy.at(0).at(1).toULongLong(), quint64(222));
    QCOMPARE(statsSpy.at(0).at(2).toBool(), true);
}

void TestConnectionManager::testInterfaceUpdatedForwarded()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);

    QSignalSpy interfaceSpy(cm_, &ConnectionManager::interfaceUpdated);
    conn->driveInterfaceUpdated("wg0");

    QTRY_COMPARE(interfaceSpy.count(), 1);
    QCOMPARE(interfaceSpy.at(0).at(0).toString(), QString("wg0"));
}

void TestConnectionManager::testTeardownBeforeRecreate()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(conn->prepareCount(), 1);
    const int createdBefore = connectionFactory_->createdCount();

    conn->driveError(CONNECT_ERROR::TCP_ERROR);

    // The failover retires the old connector (teardown before its replacement is created) and the
    // fresh attempt prepares before dialing.
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(connectionFactory_->prevTeardownCountAtCreate(), 1);
    FakeConnection *newConn = connectionFactory_->lastCreated();
    QVERIFY(newConn != conn);
    QTRY_COMPARE(newConn->prepareCount(), 1);
    QTRY_COMPARE(newConn->startConnectCount(), 1);
}

void TestConnectionManager::testStalePreparedIgnored()
{
    QPointer<FakeConnection> conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(conn->startConnectCount(), 1);

    // prepared() arriving in a state that does not expect a dial is dropped.
    conn->drivePrepared();
    QTest::qWait(20);
    QCOMPARE(conn->startConnectCount(), 1);

    // Sender guard: queue disconnected() and a stale prepared() back-to-back, then pump. The
    // disconnected handler retires this connector and creates + dials the replacement; the stale
    // prepared() from the retired sender is then delivered and must not dial the replacement again.
    conn->setAutoEmitDisconnected(false);
    conn->driveError(CONNECT_ERROR::TCP_ERROR);
    QTRY_COMPARE(cm_->state_, cm_->STATE_RECONNECTING);
    conn->driveDisconnected();
    conn->drivePrepared();

    QTRY_VERIFY(connectionFactory_->lastCreated() != nullptr && connectionFactory_->lastCreated() != conn.data());
    FakeConnection *newConn = connectionFactory_->lastCreated();
    QTRY_COMPARE(newConn->startConnectCount(), 1);
    QTest::qWait(20);
    QCOMPARE(newConn->startConnectCount(), 1);
}

void TestConnectionManager::testDisconnectDuringPrepare()
{
    // Park the attempt mid-prepare, then disconnect: the attempt cancels cleanly and the pending
    // prepare never dials (the connector is retired before its prepare could complete).
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    QPointer<FakeConnection> conn = startConnectingIkev2();
    QVERIFY(conn);
    QCOMPARE(conn->prepareCount(), 1);
    QCOMPARE(conn->startConnectCount(), 0);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QCOMPARE(connectionFactory_->teardownTotal(), 1);
    // A never-dialed connector changed no system state, so no gai/route/DNS restores are issued.
    QCOMPARE(platformPolicy_->gaiIpv4PriorityCount(), 0);
    QTest::qWait(10);
    QVERIFY(conn.isNull());
}

void TestConnectionManager::testPrepareFailedRetiresConnector()
{
    // A prepare failure with a live connector hard-stops the attempt: the connector is retired
    // (teardown + delete) and the error is surfaced.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    QPointer<FakeConnection> conn = startConnectingIkev2();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    QCOMPARE(connectionFactory_->teardownTotal(), 1);
    QTest::qWait(10);
    QVERIFY(conn.isNull());
}

void TestConnectionManager::testPrepareFailedDuringUserDisconnectIgnored()
{
    // A prepareFailed already queued when the user clicks disconnect must not hard-stop with an
    // error; the queued disconnected() completes the user-click path instead.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    QPointer<FakeConnection> conn = startConnectingIkev2();
    QVERIFY(conn);
    QCOMPARE(conn->prepareCount(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    conn->drivePrepareFailed(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testUpdateConnectionSettingsPreDialIsNoOp()
{
    // Before the dial, a settings update must leave the in-flight attempt alone (pre-Stage 4 the
    // connector did not exist yet and the update early-returned; the dialed flag preserves that).
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnectingIkev2();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTING_FROM_USER_CLICK);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testDnsAutoKeepsAdapterDnsAndEmptyOverride()
{
    // Auto DNS: the tunnel's own DNS stays on the adapter, the connector gets no override, and this
    // is the only mode that disables OS DoH.
    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    FakeConnection *conn = connectIkev2With(info);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(cm_->getVpnAdapterInfo().dnsServersAsStringList(), QStringList("10.255.255.1"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString());
    QVERIFY(dnsPlatformPolicy_->disableDohCount() >= 1);
}

void TestConnectionManager::testDnsForcedBehavesAsAuto()
{
    // Legacy FORCED (only reachable via old persisted settings) must be coerced to AUTO: no ctrld
    // listen IP may end up as the DNS override or in the leak-protection whitelist.
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_FORCED));

    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    FakeConnection *conn = connectIkev2With(info);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(cm_->getVpnAdapterInfo().dnsServersAsStringList(), QStringList("10.255.255.1"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString());
    QVERIFY(dnsConfigurator_->dnsWhitelistIps().isEmpty());
}

void TestConnectionManager::testDnsCustomIpv4OverridesAdapterDns()
{
    // Custom DNS with a plain IPv4 upstream: no ctrld involved; the upstream replaces the adapter
    // DNS and is passed to the connector as the override.
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_CUSTOM, "1.2.3.4"));

    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    FakeConnection *conn = connectIkev2With(info);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(cm_->getVpnAdapterInfo().dnsServersAsStringList(), QStringList("1.2.3.4"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString("1.2.3.4"));
    QCOMPARE(dnsPlatformPolicy_->disableDohCount(), 0);
}

void TestConnectionManager::testDnsLocalOverridesAdapterDns()
{
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_LOCAL));

    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    FakeConnection *conn = connectIkev2With(info);

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(cm_->getVpnAdapterInfo().dnsServersAsStringList(), QStringList("127.0.0.1"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString("127.0.0.1"));
    QCOMPARE(dnsPlatformPolicy_->disableDohCount(), 0);
}

void TestConnectionManager::testWireGuardTunnelDnsListAuto()
{
    // Auto DNS on WireGuard: connectingToHostname carries the connector's tunnel DNS readback and
    // the attempt environment carries no override.
    connectionFactory_->setTunnelDefaultDns("10.255.255.2");
    QSignalSpy hostnameSpy(cm_, &ConnectionManager::connectingToHostname);
    FakeConnection *conn = connectWireGuard();
    QVERIFY(conn);

    QCOMPARE(hostnameSpy.count(), 1);
    QCOMPARE(hostnameSpy.at(0).at(2).toStringList(), QStringList("10.255.255.2"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString());
}

void TestConnectionManager::testWireGuardTunnelDnsListLocal()
{
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_LOCAL));

    connectionFactory_->setTunnelDefaultDns("10.255.255.2");
    QSignalSpy hostnameSpy(cm_, &ConnectionManager::connectingToHostname);
    FakeConnection *conn = connectWireGuard();
    QVERIFY(conn);

    QCOMPARE(hostnameSpy.count(), 1);
    QCOMPARE(hostnameSpy.at(0).at(2).toStringList(), QStringList("127.0.0.1"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString("127.0.0.1"));
}

void TestConnectionManager::testWireGuardTunnelDnsListCustomIpv4()
{
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_CUSTOM, "1.2.3.4"));

    connectionFactory_->setTunnelDefaultDns("10.255.255.2");
    QSignalSpy hostnameSpy(cm_, &ConnectionManager::connectingToHostname);
    FakeConnection *conn = connectWireGuard();
    QVERIFY(conn);

    QCOMPARE(hostnameSpy.count(), 1);
    QCOMPARE(hostnameSpy.at(0).at(2).toStringList(), QStringList("1.2.3.4"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString("1.2.3.4"));
}

void TestConnectionManager::testAlwaysOnPlusCachedConfigFetchMode()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    // clickConnect recomputes the cached-config availability from stored settings (empty in the test
    // environment); with resolution parked, force the availability before the attempt proceeds.
    cm_->hasUsableCachedWgConfig_ = true;
    cm_->onHostnamesResolved();

    FakeConnection *conn = connectionFactory_->lastCreated();
    QCOMPARE(conn->prepareCount(), 1);
    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::CachedOnly);
    QCOMPARE(cm_->wgCachedConfigAttempts_, 1);
}

void TestConnectionManager::testAlwaysOnPlusCacheExhaustedAutomaticAdvances()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    connSettingsPolicyFactory_->setAutomaticMode(true);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    cm_->hasUsableCachedWgConfig_ = true;
    cm_->wgCachedConfigAttempts_ = cm_->kMaxWgCachedConfigAttempts;

    FakeConnection *conn = connectionFactory_->lastCreated();
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->onHostnamesResolved();

    // The budget is spent: let the policy advance to a protocol that works without the API,
    // without ever preparing the connector.
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(conn->prepareCount(), 0);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testAlwaysOnPlusCacheExhaustedManualAborts()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    connSettingsPolicyFactory_->setAutomaticMode(false);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // No usable cached config at all; manual mode has no next protocol to advance to.
    cm_->onHostnamesResolved();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
    QTRY_COMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testAlwaysOnPlusLeavesNonFetchingConnectorAlone()
{
    // The gate is keyed on the connector's supportsCachedConfig capability, so an IKEv2 attempt
    // under Always On+ prepares normally.
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    FakeConnection *conn = connectIkev2();
    QVERIFY(conn);

    QCOMPARE(conn->prepareCount(), 1);
    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::Normal);
    QCOMPARE(cm_->wgCachedConfigAttempts_, 0);
}

void TestConnectionManager::testDnsControldStartsCtrldAndUsesListenIp()
{
    // Control D DNS: prepare() starts ctrld with the single upstream, and the ctrld listen address
    // becomes both the adapter DNS override and the connector's override.
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());

    FakeConnection *conn = connectIkev2();

    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(ctrldManager_->runCount(), 1);
    QCOMPARE(ctrldManager_->lastUpstream1(), QString("https://dns.controld.com/abcd1234"));
    QCOMPARE(ctrldManager_->lastUpstream2(), QString());
    QCOMPARE(ctrldManager_->lastDomains(), QStringList());
    QCOMPARE(cm_->getVpnAdapterInfo().dnsServersAsStringList(), QStringList("127.0.0.10"));
    QCOMPARE(conn->lastPrepareEnv().primaryDnsServer, QString("127.0.0.10"));
}

void TestConnectionManager::testDnsCtrldStartFailureAbortsConnect()
{
    // ctrld failing to start is fatal for the attempt: the error is surfaced and nothing dials.
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());
    ctrldManager_->setRunResult(false);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), CONNECT_ERROR::CTRLD_START_FAILED);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
    // Stage 4 (early creation): the connector now exists before the DNS prep step runs, so the
    // abort leaves an idle connector behind instead of never creating one; it must not have dialed.
    QCOMPARE(connectionFactory_->createdCount(), 1);
    QCOMPARE(connectionFactory_->lastCreated()->startConnectCount(), 0);
}

void TestConnectionManager::testDnsCtrldKilledOnUserDisconnect()
{
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());

    connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    const int killsBefore = ctrldManager_->killCount();

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QVERIFY(ctrldManager_->killCount() > killsBefore);
}

void TestConnectionManager::testDnsCtrldKilledOnReconnect()
{
    // A reconnect kills the running ctrld before the next attempt's prepare() starts a fresh one.
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());

    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTED);
    QCOMPARE(ctrldManager_->runCount(), 1);
    const int killsBefore = ctrldManager_->killCount();

    conn->driveError(CONNECT_ERROR::TCP_ERROR);

    // The kill happens in the queued onConnectionDisconnected, after the state already flipped.
    QTRY_VERIFY(ctrldManager_->killCount() > killsBefore);
    QTRY_COMPARE(ctrldManager_->runCount(), 2);
    QCOMPARE(cm_->state_, cm_->STATE_RECONNECTING);
}

void TestConnectionManager::testDnsSplitDnsPassesUpstreamsAndHostnames()
{
    // Split DNS routes through ctrld even with an IPv4 upstream, passing both upstreams and the
    // domain list; the tunnel then uses the upstream plus the ctrld listen address.
    types::ConnectedDnsInfo dnsInfo = makeDnsInfo(CONNECTED_DNS_TYPE_CUSTOM, "1.2.3.4");
    dnsInfo.isSplitDns = true;
    dnsInfo.upStream2 = "5.6.7.8";
    dnsInfo.hostnames = QStringList() << "a.example.com" << "b.example.com";
    dnsConfigurator_->setConnectedDnsInfo(dnsInfo);

    QVERIFY(dnsConfigurator_->prepare());
    QCOMPARE(ctrldManager_->runCount(), 1);
    QCOMPARE(ctrldManager_->lastUpstream1(), QString("1.2.3.4"));
    QCOMPARE(ctrldManager_->lastUpstream2(), QString("5.6.7.8"));
    QCOMPARE(ctrldManager_->lastDomains(), QStringList() << "a.example.com" << "b.example.com");
    QCOMPARE(dnsConfigurator_->primaryDnsServer(), QString("127.0.0.10"));
    QCOMPARE(dnsConfigurator_->tunnelDnsServers("10.255.255.2"), QStringList() << "1.2.3.4" << "127.0.0.10");
    QCOMPARE(dnsConfigurator_->dnsWhitelistIps(), QStringList() << "127.0.0.10" << "1.2.3.4" << "5.6.7.8");
}

void TestConnectionManager::testDnsDohUpstreamQueries()
{
    // A DoH upstream is not an IP, so only the ctrld listen address is usable by the tunnel.
    dnsConfigurator_->setConnectedDnsInfo(makeDnsInfo(CONNECTED_DNS_TYPE_CUSTOM, "https://dns.example.com/query"));

    QVERIFY(dnsConfigurator_->prepare());
    QCOMPARE(dnsConfigurator_->primaryDnsServer(), QString("127.0.0.10"));
    QCOMPARE(dnsConfigurator_->tunnelDnsServers("10.255.255.2"), QStringList("127.0.0.10"));
    QCOMPARE(dnsConfigurator_->dnsWhitelistIps(), QStringList("127.0.0.10"));

    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    dnsConfigurator_->overrideAdapterDns(info);
    QCOMPARE(info.dnsServersAsStringList(), QStringList("127.0.0.10"));
}

QTEST_MAIN(TestConnectionManager)
