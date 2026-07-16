#include <QSettings>
#include <QSignalSpy>
#include <QtTest>

#include "connectionmanager.test.h"

#include <memory>

#include "engine/connectionmanager/connectionmanager.h"
#include "engine/connectionmanager/isleepevents.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include "engine/helper/helper.h"
#include "engine/wireguardconfig/getwireguardconfig.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "fakes.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/proxysettings.h"
#include "api_responses/portmap.h"
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

    // CM takes ownership of the seam objects.
    cm_ = new ConnectionManager(nullptr, helper_, networkDetectionManager_, credentialsStorage_,
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
    // The seam objects are owned by cm_ and already gone.
    delete credentialsStorage_;
    delete networkDetectionManager_;
    delete helper_;
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
    d.dnsHostName = "ikev2.example.com";
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

FakeConnection *TestConnectionManager::startConnectingIkev2()
{
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    return connectionFactory_->lastCreated();
}

FakeConnection *TestConnectionManager::connectIkev2()
{
    FakeConnection *conn = startConnectingIkev2();
    conn->driveConnected(vpnAdapterInfo());
    for (int i = 0; i < 200 && cm_->state_ != cm_->STATE_CONNECTED; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }
    return conn;
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
    QVERIFY(platformPolicy_->disableDohCount() >= 1);
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
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    QCOMPARE(cm_->state_, cm_->STATE_CONNECTING_FROM_USER_CLICK);

    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    cm_->onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kKeyLimit, WireGuardConfig());

    QCOMPARE(keyLimitSpy.count(), 1);
    // Timeout is paused while awaiting the user's key-limit decision.
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testWireGuardKeyLimitDecline()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);
    cm_->onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kKeyLimit, WireGuardConfig());

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->onWireGuardKeyLimitUserResponse(false);

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testWireGuardConfigFetchFailedManual()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutomaticMode(false);
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    cm_->onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kFailoverFailed, WireGuardConfig());

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(errorSpy.at(0).at(0)), WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
    QCOMPARE(cm_->state_, cm_->STATE_DISCONNECTED);
}

void TestConnectionManager::testWireGuardConfigFetchFailedAutomatic()
{
    connSettingsPolicyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connSettingsPolicyFactory_->setAutomaticMode(true);
    connSettingsPolicyFactory_->setAutoResolveHostnames(false);
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    cm_->clickConnect(req);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Automatic mode retries (lets the policy advance) rather than surfacing a fatal error.
    cm_->onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kFailoverFailed, WireGuardConfig());

    QVERIFY(reconnectingSpy.count() >= 1);
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
    QCOMPARE(conn->continueWithUsernameAndPasswordCount(), 1);
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
    conn->driveRequestUsername();

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
    conn->driveRequestPassword();

    QTRY_COMPARE(passwordSpy.count(), 1);
    QCOMPARE(passwordSpy.at(0).at(0).toString(), QString("/configs/my.ovpn"));
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

QTEST_MAIN(TestConnectionManager)
