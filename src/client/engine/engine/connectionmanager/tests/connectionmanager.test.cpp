#include <QSettings>
#include <QSignalSpy>
#include <QtTest>

#include "connectionmanager.test.h"

#include <memory>
#include <variant>

#include "api_responses/portmap.h"
#include "engine/connectionmanager/classifyconnecterror.h"
#include "engine/connectionmanager/connectionmanager.h"
#include "engine/connectionmanager/connectors/connectionfactory.h"
#include "engine/connectionmanager/isleepevents.h"
#include "engine/dns/dnsconfigurator.h"
#include "engine/helper/helper.h"
#include "extraconfig_mock.h"
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
    qRegisterMetaType<ConnectError>("ConnectError");
    qRegisterMetaType<DISCONNECT_REASON>("DISCONNECT_REASON");
    qRegisterMetaType<CurrentConnectionDescr>("CurrentConnectionDescr");
    qRegisterMetaType<types::Protocol>("types::Protocol");
    qRegisterMetaType<QVector<types::ProtocolStatus>>("QVector<types::ProtocolStatus>");
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
    connectionFactory_ = new FakeConnectionFactory();
    platformPolicy_ = new FakePlatformPolicy();
    attemptStrategyFactory_ = new FakeConnectionAttemptStrategyFactory();
    sleepEvents_ = new FakeSleepEvents();
    ExtraConfigMock::reset();
    dnsPlatformPolicy_ = new FakePlatformPolicy();
    ctrldManager_ = new FakeCtrldManager();
    dnsConfigurator_ = new DnsConfigurator(nullptr, helper_, dnsPlatformPolicy_, ctrldManager_);

    // CM takes ownership of the seam objects; the DnsConfigurator stays caller-owned, as in Engine.
    cm_ = new ConnectionManager(nullptr, networkDetectionManager_, dnsConfigurator_,
                                connectionFactory_, platformPolicy_, attemptStrategyFactory_, sleepEvents_);

    // Default: an automatic-mode IKEv2 default node. IKEv2 is the simplest happy-path protocol because
    // it needs no external process, ovpn file generation, or WireGuard config fetch.
    attemptStrategyFactory_->setCurrentConnectionSettings(makeIkev2Descr());
    attemptStrategyFactory_->setAutomaticMode(true);
}

void TestConnectionManager::cleanup()
{
    delete cm_;
    cm_ = nullptr;
    // The seam objects are owned by cm_ and already gone. The DnsConfigurator owns its policy and
    // ctrld manager.
    delete dnsConfigurator_;
    delete networkDetectionManager_;
    delete helper_;
    dnsConfigurator_ = nullptr;
    dnsPlatformPolicy_ = nullptr;
    ctrldManager_ = nullptr;
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

FakeConnection *TestConnectionManager::startConnecting()
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
    FakeConnection *conn = startConnecting();
    conn->driveConnected(info);
    // A timeout here surfaces through the callers' state assertions.
    (void)QTest::qWaitFor([this] { return cm_->state_ == ConnectionManager::State::kConnected; });
    return conn;
}

FakeConnection *TestConnectionManager::connectWireGuard()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    // Stage 4 (uniformly async prepare): the dial happens one event-loop hop after prepared().
    FakeConnection *conn = startConnecting();
    // A timeout here surfaces through the callers' startConnectCount assertions.
    (void)QTest::qWaitFor([conn] { return conn->startConnectCount() > 0; });
    return conn;
}

FakeConnection *TestConnectionManager::startConnectingWireGuardAtKeyLimit()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitUserInputRequired);
    connectionFactory_->setPrepareInputType(UserInputType::KeyLimitConsent);
    return startConnecting();
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

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(hostnameSpy.count(), 1);
    // The payload comes from the connector's effective* readbacks (which default to the descr).
    QCOMPARE(hostnameSpy.at(0).at(0).toString(), QString("ikev2.example.com"));
    QCOMPARE(hostnameSpy.at(0).at(1).toString(), QString("10.0.0.2"));
    // No connector-carried tunnel DNS (non-WireGuard): the firewall whitelist gets an empty list.
    QCOMPARE(hostnameSpy.at(0).at(2).toStringList(), QStringList());
    QCOMPARE(conn->startConnectCount(), 1);
    QVERIFY(!cm_->isStaticIpsLocation());
    QVERIFY(!cm_->connectingTimer_.isActive());
    // Engine reads the pre-connect default adapter for firewall/whitelist decisions.
    QCOMPARE(cm_->getDefaultAdapterInfo().adapterName(), QString("fake0"));
    // The connect path applies its platform tweaks: DoH disabled for auto-DNS and IPv4 gai priority
    // enabled for the (non-IPv6) IKEv2 tunnel.
    QVERIFY(dnsPlatformPolicy_->disableDohCount() >= 1);
    QVERIFY(platformPolicy_->lastGaiIpv4Priority());
}

void TestConnectionManager::testTunnelTestPassAfterConnect()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Pretend an earlier cached-WG failover consumed the budget.
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->setCachedConfigAvailability(true);
    strategy->exhaustCachedConfigBudget();

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(true, "203.0.113.7");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), true);
    QCOMPARE(testTunnelSpy.at(0).at(1).toString(), QString("203.0.113.7"));
    // Automatic mode records the successful attempt so a later failover won't immediately give up.
    QVERIFY(cm_->bAttemptSucceeded_);
    // A working tunnel refreshes the cached-WG budget so subsequent reconnects can retry it.
    QCOMPARE(strategy->cachedConfigAttempts(), 0);
}

void TestConnectionManager::testUserDisconnectFromConnected()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    // The connection strategy is reset on a user disconnect.
    QVERIFY(attemptStrategyFactory_->lastCreated()->resetCount() >= 1);
}

void TestConnectionManager::testUserDisconnectFromConnecting()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    // Read before the settle: the retired connector dies with the event pump below.
    QCOMPARE(conn->startDisconnectCount(), 1);

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testUserDisconnectWhenNoConnector()
{
    // No connect attempt: clickDisconnect from the disconnected state with no connector emits
    // disconnected() synchronously.
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect(DISCONNECTED_BY_USER);

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testFailoverRetriesWhenNotExhausted()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveError(ConnectError::kTransientTunnelFailure);

    // Not exhausted -> reconnect: reconnecting() emitted exactly once and a fresh connector is created.
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testFailoverExhaustedEmitsDisconnectedItself()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    attemptStrategyFactory_->lastCreated()->setFailed(true);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn->driveError(ConnectError::kTransientTunnelFailure);

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    // The failed attempt was recorded on the strategy before giving up.
    QVERIFY(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount() >= 1);
}

void TestConnectionManager::testErrorClassificationAuthErrorFatalWhenEmit()
{
    // A custom-OVPN descriptor so we can also pin that errorDuringConnection carries it (Engine
    // branches its auth handling off the descriptor's node type + protocol).
    CurrentConnectionDescr descr;
    descr.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    descr.protocol = types::Protocol(types::Protocol::OPENVPN_UDP);
    attemptStrategyFactory_->setCurrentConnectionSettings(descr);

    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    req.bEmitAuthError = true;
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);

    // With bEmitAuthError the auth error is fatal but is surfaced only once the connector confirms
    // it stopped.
    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(errorSpy.count(), 0);
    // Auth is exempt from the forced teardown: the connector emits disconnected() on its own.
    QCOMPARE(conn->startDisconnectCount(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kAuthFailure);
    // The descriptor rides the signal so Engine can classify the auth error without querying CM.
    const auto emittedDescr = qvariant_cast<CurrentConnectionDescr>(errorSpy.at(0).at(1));
    QCOMPARE(emittedDescr.connectionNodeType, CONNECTION_NODE_CUSTOM_CONFIG);
    QVERIFY(emittedDescr.protocol.isOpenVpnProtocol());
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testProcessNotRespondingSettlesViaForcedDisconnect()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);

    // The management-socket failure paths pre-settle the connector, which then never self-emits
    // disconnected(); CM must force the teardown (startDisconnect() on a settled connector emits it
    // immediately — the fake's auto-emit models that fast path) or the machine wedges in kStopping.
    conn->driveError(ConnectError::kLocalProcessNotResponding);

    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kLocalProcessNotResponding);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testErrorClassificationImmediateStop()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    conn->driveError(ConnectError::kLocalProcessLaunchFailure);

    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kLocalProcessLaunchFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testAuthErrorRetryableWhenNotEmit()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Without bEmitAuthError the auth error is retryable; CM waits for the connector's own
    // disconnected() rather than forcing a disconnect.
    conn->driveError(ConnectError::kAuthFailure);

    QTRY_COMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testManualModeWireGuardErrorIsFatal()
{
    // Classification depends on error code + mode, not on the live connector's protocol, so an IKEv2
    // connector in manual mode faithfully exercises the WireGuard-error branch.
    attemptStrategyFactory_->setAutomaticMode(false);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // Fatal: CM parks in the error state and surfaces errorDuringConnection only once the connector
    // confirms it stopped (same deferral as the fatal auth-error path).
    conn->driveError(ConnectError::kTunnelEstablishmentFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(errorSpy.count(), 0);
    // Non-auth fatal errors force the teardown; a pre-settled connector would never emit
    // disconnected() on its own.
    QCOMPARE(conn->startDisconnectCount(), 1);

    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kTunnelEstablishmentFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testAutomaticModeWireGuardErrorIsRetryable()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveError(ConnectError::kTunnelEstablishmentFailure);

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testWireGuardErrorClearsCachedWgConfigWhenExhausted()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    // The clear is gated on the erroring connector actually being a config-fetching (WG) one.
    conn->setSupportsCachedConfig(true);
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->setCachedConfigAvailability(true);
    strategy->exhaustCachedConfigBudget();

    conn->driveError(ConnectError::kTunnelEstablishmentFailure);

    QTRY_VERIFY(!strategy->hasUsableCachedConfig());
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 1);
}

void TestConnectionManager::testConnectionFactoryStoredConfigOps()
{
    // The real storage path behind the seam: removal actually deletes the stored key, and an
    // undecryptable or absent value is never reported usable. (The usable-true path needs the
    // private encrypted serializer and stays uncovered, as it was before the seam.)
    QSettings settings;
    settings.setValue("wireguardConfig", "stale");

    ConnectionFactory factory(helper_);
    QVERIFY(!factory.hasUsableStoredConfig());
    factory.removeStoredConfig();
    QVERIFY(!settings.contains("wireguardConfig"));
    QVERIFY(!factory.hasUsableStoredConfig());
}

void TestConnectionManager::testClassifyConnectErrorMatrix()
{
    using C = ConnectErrorClassification;

    // (err, isAutomaticMode, emitAuthError, retryOnAttemptFailure) -> classification.
    QCOMPARE(classifyConnectError(ConnectError::kAuthFailure, false, true, false), C::ErrorAfterDisconnect);
    QCOMPARE(classifyConnectError(ConnectError::kAuthFailure, false, false, false), C::Retry);

    QCOMPARE(classifyConnectError(ConnectError::kLocalProcessNotResponding, false, false, false), C::ErrorAfterDisconnect);
    QCOMPARE(classifyConnectError(ConnectError::kLocalProcessNotResponding, false, false, true), C::Retry);
    QCOMPARE(classifyConnectError(ConnectError::kLocalProcessLaunchFailure, false, false, false), C::ErrorImmediately);
    QCOMPARE(classifyConnectError(ConnectError::kLocalProcessLaunchFailure, false, false, true), C::Retry);

    // WireGuard adapter setup failure: an endpoint-list walk continues; otherwise automatic mode
    // fails over to another protocol and manual mode is fatal.
    QCOMPARE(classifyConnectError(ConnectError::kAdapterSetupFailure, false, false, true), C::Retry);
    QCOMPARE(classifyConnectError(ConnectError::kAdapterSetupFailure, true, false, false), C::Retry);
    QCOMPARE(classifyConnectError(ConnectError::kAdapterSetupFailure, false, false, false), C::ErrorAfterDisconnect);

    // A missing adapter driver is fatal in both automatic and manual mode (failing over cannot
    // help); only an endpoint-list walk continues.
    QCOMPARE(classifyConnectError(ConnectError::kAdapterNotInstalled, false, false, true), C::Retry);
    QCOMPARE(classifyConnectError(ConnectError::kAdapterNotInstalled, true, false, false), C::ErrorAfterDisconnect);
    QCOMPARE(classifyConnectError(ConnectError::kAdapterNotInstalled, false, false, false), C::ErrorAfterDisconnect);

    QCOMPARE(classifyConnectError(ConnectError::kTunnelEstablishmentFailure, false, false, false), C::ErrorAfterDisconnect);
    QCOMPARE(classifyConnectError(ConnectError::kTunnelEstablishmentFailure, true, false, false), C::Retry);
    // The endpoint-list exemption covers adapter setup only; establishment failure follows the mode
    // even mid-walk.
    QCOMPARE(classifyConnectError(ConnectError::kTunnelEstablishmentFailure, false, false, true), C::ErrorAfterDisconnect);

    QCOMPARE(classifyConnectError(ConnectError::kPrivKeyPasswordFailure, true, false, false), C::ErrorAfterDisconnect);

    QCOMPARE(classifyConnectError(ConnectError::kVpnServiceSetupFailure, false, false, false), C::ErrorImmediately);
    QCOMPARE(classifyConnectError(ConnectError::kVpnServiceSetupFailure, true, false, false), C::Retry);
    QCOMPARE(classifyConnectError(ConnectError::kHostsFileNotWritable, false, false, false), C::ErrorImmediately);
    QCOMPARE(classifyConnectError(ConnectError::kHostsFileNotWritable, true, false, false), C::Retry);

    QCOMPARE(classifyConnectError(ConnectError::kTransientTunnelFailure, false, false, false), C::Retry);

    // Prepare-phase errors are routed by the prepareFailed table, never by the dial-phase classifier.
    QCOMPARE(classifyConnectError(ConnectError::kConfigFetchFailure, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kNoError, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kLocationUnavailable, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kAccountBlocked, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kCustomConfigInvalid, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kLocalConfigGenerationFailure, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kDnsServiceStartFailure, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kBlockedByOsPolicy, true, false, false), C::Unknown);
    QCOMPARE(classifyConnectError(ConnectError::kLocalDnsServerNotAvailable, true, false, false), C::Unknown);
}

void TestConnectionManager::testRecordFailureAndAdvance()
{
    FakeConnectionAttemptStrategy strategy;

    // Not exhausted -> next attempt, no reset.
    QCOMPARE(strategy.recordFailureAndAdvance(false), IConnectionAttemptStrategy::FailureAdvice::Retry);
    QCOMPARE(strategy.putFailedConnectionCount(), 1);
    QCOMPARE(strategy.resetCount(), 0);

    // Exhausted without an earlier success -> give up.
    strategy.setFailed(true);
    QCOMPARE(strategy.recordFailureAndAdvance(false), IConnectionAttemptStrategy::FailureAdvice::GiveUp);
    QCOMPARE(strategy.resetCount(), 0);

    // Exhausted after a success -> reset the walk and start over, with a fresh cached-config budget.
    strategy.setCachedConfigAvailability(true);
    strategy.exhaustCachedConfigBudget();
    QCOMPARE(strategy.recordFailureAndAdvance(true), IConnectionAttemptStrategy::FailureAdvice::Retry);
    QCOMPARE(strategy.resetCount(), 1);
    QCOMPARE(strategy.cachedConfigAttempts(), 0);
}

void TestConnectionManager::testAttemptSucceededBankedOnlyInAutomaticMode()
{
    // Manual mode never banks a success: an exhausted manual walk must surface its failure.
    attemptStrategyFactory_->setAutomaticMode(false);
    connectIkev2();
    cm_->onTunnelTestResult(true, "203.0.113.7");
    QVERIFY(!cm_->bAttemptSucceeded_);

    cm_->clickDisconnect();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    attemptStrategyFactory_->setAutomaticMode(true);
    connectIkev2();
    cm_->onTunnelTestResult(true, "203.0.113.7");
    QVERIFY(cm_->bAttemptSucceeded_);

    // A fresh connect starts a new session: the banked success must not leak into it.
    cm_->clickDisconnect();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    connectIkev2();
    QVERIFY(!cm_->bAttemptSucceeded_);
}

void TestConnectionManager::testReconnectionTimerExpiry()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // Simulate the 1-hour reconnection cap firing.
    cm_->onTimerReconnection();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testConnectingTimeout()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // The connecting-timeout slot behaves as if the single-shot timer expired.
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(reconnectingSpy.count(), 1);
    // The timeout is a real attempt failure: it consumes an endpoint from the strategy walk.
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testConnectingTimeoutFiredByTimerFailsOver()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_VERIFY(conn->startConnectCount() > 0);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);

    // Fire the real connecting timer through the event loop instead of invoking the slot directly:
    // only then does sender() point at the timer while onConnectingTimeout() runs. The failover it
    // triggers must still proceed -- reading sender() there would mistake it for a stale connector
    // signal and silently drop the reconnect, stranding the attempt.
    cm_->connectingTimer_.stop();
    cm_->connectingTimer_.setSingleShot(true);
    cm_->connectingTimer_.start(1);

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testConnectingTimeoutClearsCachedWgConfigWhenExhausted()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    // A cached config with stale credentials starts and configures fine but never completes the
    // handshake, so the failure surfaces as a connecting timeout, not a connector error.
    conn->setSupportsCachedConfig(true);
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->setCachedConfigAvailability(true);
    strategy->exhaustCachedConfigBudget();

    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QVERIFY(!strategy->hasUsableCachedConfig());
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testConnectingTimeoutKeepsCachedWgConfig()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();
    // Below the attempt cap a timeout may be transient; the config must survive it.
    conn->setSupportsCachedConfig(true);
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->setCachedConfigAvailability(true);
    for (int i = 0; i < IConnectionAttemptStrategy::kMaxCachedConfigAttempts - 1; ++i) {
        strategy->takeCachedConfigAdvice();
    }

    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    QVERIFY(strategy->hasUsableCachedConfig());
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 0);

    // A non-config-fetching connector timing out later in the rotation must not discard the config
    // either, even with the budget exhausted (the failover created a fresh connector without the
    // cached-config capability).
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    strategy->exhaustCachedConfigBudget();
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    // Budget exhausted and the config still usable: only a cached-config failure may clear it.
    QVERIFY(strategy->hasUsableCachedConfig());
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 0);
}

void TestConnectionManager::testNetworkOfflineWhileConnected()
{
    // The platform policy opts the state machine into reacting to online-state changes (macOS in
    // production); the fake forces it on so the branch is exercised on every platform.
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Keep the connector alive so the post-offline state is observable deterministically.
    conn->setAutoEmitDisconnected(false);

    QSignalSpy connectivitySpy(cm_, &ConnectionManager::internetConnectivityChanged);
    networkDetectionManager_->setOnline(false);

    QCOMPARE(connectivitySpy.count(), 1);
    QCOMPARE(connectivitySpy.at(0).at(0).toBool(), false);

    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), 1);
}

void TestConnectionManager::testNetworkOfflineIgnoredWhenPolicyOptsOut()
{
    // Win/Linux in production: the connectivity signal is still forwarded, but the state machine
    // does not react.
    platformPolicy_->setReconnectOnOnlineStateChange(false);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    QSignalSpy connectivitySpy(cm_, &ConnectionManager::internetConnectivityChanged);
    networkDetectionManager_->setOnline(false);

    QCOMPARE(connectivitySpy.count(), 1);
    QCOMPARE(connectivitySpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testSleepWakeReconnect()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    sleepEvents_->driveSleep();

    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);

    const int createdBefore = connectionFactory_->createdCount();
    sleepEvents_->driveWake();

    // Wake restores the connection: the wake-flavored reconnect creates a fresh connector and dials it.
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
}

void TestConnectionManager::testWakeDuringBlockingDisconnect()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    // Model a wake event queued while blockingDisconnect() is still spinning. On non-Windows the loop
    // pumps the event loop, so onWakeMode() runs mid-loop and reconnects via restoreConnectionAfterWakeUp().
    // On Windows the loop deliberately pumps nothing: waitForDisconnect() blocks until the stop completes,
    // and the queued wake is only delivered after onSleepMode() returns, reconnecting from the sleep
    // state. Either route lands in the wake-flavored Reconnecting state rather than parking in Sleeping.
    conn->setAutoEmitDisconnected(false);
    QTimer::singleShot(0, sleepEvents_, [this]() { sleepEvents_->driveWake(); });
#ifndef Q_OS_WIN
    QTimer::singleShot(0, conn, [conn]() { conn->driveDisconnected(); });
#endif

    sleepEvents_->driveSleep();

    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
}

void TestConnectionManager::testWireGuardKeyLimitPrompt()
{
    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    startConnectingWireGuardAtKeyLimit();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);

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
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
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
    // connected()/error()/reconnecting() from a retired connector, queued behind its disconnected(),
    // must not act on the manager (they would resurrect the session / count failures / stop the DNS
    // proxy against a later attempt).
    FakeConnection *conn = startConnecting();
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    QSignalSpy statisticsSpy(cm_, &ConnectionManager::statisticsUpdated);
    QSignalSpy interfaceSpy(cm_, &ConnectionManager::interfaceUpdated);
    conn->driveDisconnected();
    conn->driveError(ConnectError::kTransientTunnelFailure);
    conn->driveReconnecting();
    conn->driveConnected(vpnAdapterInfo());
    conn->driveStatisticsUpdated(1, 2, false);
    conn->driveInterfaceUpdated("utun9");

    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QTest::qWait(10);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(statisticsSpy.count(), 0);
    QCOMPARE(interfaceSpy.count(), 0);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testSleepDuringPrepareQuiescesAttempt()
{
    // Sleep landing mid-prepare tears the pre-dial attempt down (wrappers/fetches quiesced) and
    // wake restores it from scratch.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QCOMPARE(conn->startConnectCount(), 0);

    sleepEvents_->driveSleep();

    QCOMPARE(conn->teardownCount(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);

    sleepEvents_->driveWake();
    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
}

void TestConnectionManager::testWireGuardKeyLimitIgnoredDuringDisconnect()
{
    // A key-limit request already queued when the user starts disconnecting must not raise the prompt.
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    conn->setAutoEmitDisconnected(false);

    QSignalSpy keyLimitSpy(cm_, &ConnectionManager::wireGuardAtKeyLimit);
    cm_->clickDisconnect();
    conn->driveUserInputRequired(UserInputType::KeyLimitConsent);
    QTest::qWait(10);
    QCOMPARE(keyLimitSpy.count(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
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
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    cm_->onWireGuardKeyLimitUserResponse(true);
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testRetryPolicyWalksOnBareDisconnect()
{
    // An endpoint-list strategy consumes the endpoint and dials the next one when the process dies
    // without a classified error.
    attemptStrategyFactory_->setRetryOnAttemptFailure(true);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnecting();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveDisconnected();

    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
}

void TestConnectionManager::testRetryPolicyBareDisconnectExhaustedStops()
{
    attemptStrategyFactory_->setRetryOnAttemptFailure(true);
    attemptStrategyFactory_->setFailed(true);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnecting();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testRetryPolicyWalksOnFatalError()
{
    // Local fatal errors (spawn/socket/adapter) walk the endpoint list instead of ending the attempt.
    attemptStrategyFactory_->setRetryOnAttemptFailure(true);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    FakeConnection *conn = startConnecting();
    QTRY_COMPARE(conn->startConnectCount(), 1);

    conn->driveError(ConnectError::kLocalProcessLaunchFailure);

    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(errorSpy.count(), 0);
    // The error consumed the endpoint; the connector's follow-up disconnected() must not consume
    // a second one (the ignore-errors dedupe in the kReconnecting disconnect arm).
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
}

void TestConnectionManager::testBlockingDisconnectQuiescesPreDialAttempt()
{
    // A pre-dial attempt (prepare in flight) is fully cancelled by blockingDisconnect: teardown runs
    // and the manager ends up disconnected without emitting.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnecting();
    QCOMPARE(conn->startConnectCount(), 0);

    cm_->blockingDisconnect(false);

    QCOMPARE(conn->teardownCount(), 1);
    QVERIFY(cm_->isDisconnected());
    QCOMPARE(disconnectedSpy.count(), 0);
}

void TestConnectionManager::testWireGuardConfigFetchFailedManual()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutomaticMode(false);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(ConnectError::kConfigFetchFailure);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    startConnecting();

    // Manual mode has no next option to advance to, so the API-exhausted failure is fatal.
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kConfigFetchFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testWireGuardConfigFetchFailedAutomatic()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutomaticMode(true);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(ConnectError::kConfigFetchFailure);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Automatic mode retries (lets the strategy advance) rather than surfacing a fatal error.
    startConnecting();

    QTRY_VERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testConfigFetchFailedFailsOverInManualMode()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutomaticMode(false);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::EmitPrepareFailed);
    connectionFactory_->setPrepareFailure(ConnectError::kLocalConfigGenerationFailure);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    startConnecting();

    // Unlike the API-exhausted code, a plain fetch failure routes to failover in BOTH modes.
    QTRY_VERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testUpdateConnectionSettingsWhileConnectedReconnects()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testUpdateConnectionSettingsPreservesAttemptSucceeded()
{
    connectIkev2();
    cm_->onTunnelTestResult(true, "203.0.113.7");
    QVERIFY(cm_->bAttemptSucceeded_);
    QPointer<FakeConnectionAttemptStrategy> oldPolicy = attemptStrategyFactory_->lastCreated();

    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    // The banked success is session state owned by CM, so the strategy recreation can't shorten the
    // retry budget from 1 hour to a single walk.
    QVERIFY(oldPolicy.isNull());
    QVERIFY(cm_->bAttemptSucceeded_);
}

void TestConnectionManager::testContinueWithUsernamePasswordForwards()
{
    FakeConnection *conn = startConnecting();
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
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    cm_->continueWithUsernameAndPassword("user", "pass", true);

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QVERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testSpontaneousDropFromConnectedReconnects()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // Connector drops on its own (not via an error) while connected -> CM reconnects.
    conn->driveDisconnected();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testReconnectPublicMethod()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->reconnect();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testTunnelTestFailedRetriesWhenNotExhausted()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->onTunnelTestResult(false, "");

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testTunnelTestFailedExhaustedDisconnects()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    attemptStrategyFactory_->lastCreated()->setFailed(true);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->onTunnelTestResult(false, "");

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testTunnelTestResultIgnoredWhenNotConnected()
{
    // A tunnel-test result racing in behind a disconnect must be inert: the Engine-owned tester and
    // the disconnect can cross, and acting on the stale result would hijack the stop into a failover.
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    cm_->clickDisconnect();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    const int createdBefore = connectionFactory_->createdCount();
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(false, "");

    QTest::qWait(10);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(testTunnelSpy.count(), 0);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testTunnelTestFailedCustomConfigEmitsResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Custom configs surface the tunnel-test failure to the UI instead of failing over.
    attemptStrategyFactory_->lastCreated()->setCustomConfig(true);

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
}

void TestConnectionManager::testTunnelTestNoErrorFlagSurfacesResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // The ExtraConfig no-error flag surfaces the failure to the UI without failing over, regardless
    // of mode or custom-config. Drive it through the tunnel-test seam so the branch is pinned
    // rather than depending on an on-disk config.
    ExtraConfigMock::tunnelTestNoError = true;

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
}

void TestConnectionManager::testProtocolChangedStartsRestTimer()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    // A protocol change inserts a 10s rest between failover attempts rather than reconnecting at once.
    attemptStrategyFactory_->lastCreated()->setProtocolChanged(true);
    const int createdBefore = connectionFactory_->createdCount();

    conn->driveError(ConnectError::kTransientTunnelFailure);

    QTRY_VERIFY(cm_->connectTimer_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    // The rest timer expiring triggers the next attempt.
    cm_->connectTimer_.stop();
    cm_->onConnectTrigger();
    QVERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testWakeWhileOfflineWithoutConnectorPollsAndReconnects()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    conn->setAutoEmitDisconnected(false);

    // Park offline and let the connector die, so nothing dialed survives into the sleep.
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    conn->driveDisconnected();
    QTRY_VERIFY(cm_->timerWaitNetworkConnectivity_.isActive());

    // Sleep must quiesce the poll: a tick firing mid-sleep would dial while the machine sleeps.
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());

    // Waking still offline parks with no dialed connector, so the online-state slot can never drive
    // the recovery; the poll (under the reconnection cap) must be re-armed instead.
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QVERIFY(cm_->timerReconnection_.isActive());

    const int createdBefore = connectionFactory_->createdCount();
    networkDetectionManager_->setOnline(true);
    cm_->onTimerWaitNetworkConnectivity();
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), createdBefore + 1);

    // The redial must leave the park, or its own connected() is dropped as stale.
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    connectionFactory_->lastCreated()->driveConnected(vpnAdapterInfo());
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(connectedSpy.count(), 1);
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testConnectorReconnectingSignalFromConnected()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Keep the connector alive so the reconnecting transition is observable before teardown.
    conn->setAutoEmitDisconnected(false);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // The connector itself reports it is reconnecting (e.g. OpenVPN mid-session) -> CM moves to
    // RECONNECTING, arms the reconnection cap, and tells the connector to stop.
    conn->driveReconnecting();

    QTRY_COMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), 1);
}

void TestConnectionManager::testOnlineButNoDefaultAdapterWaitsForNetwork()
{
    // Online, but the OS reports no usable default adapter yet: startAttempt parks and polls for
    // connectivity instead of attempting the connection.
    platformPolicy_->setDefaultAdapter(AdapterGatewayInfo());
    startConnecting();

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
    // A strategy that opts out of waiting for connectivity (emergency connect) surfaces the failure
    // immediately instead of parking in the wait-for-network poll.
    attemptStrategyFactory_->setWaitForNetwork(false);
    networkDetectionManager_->setOnline(false);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    startConnecting();

    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), 0);
}

void TestConnectionManager::testLockdownModeIkev2Error()
{
    // Manual-mode IKEv2 while lockdown mode blocks IKEv2: startAttempt surfaces a fatal error and never
    // creates a connector.
    attemptStrategyFactory_->setAutomaticMode(false);
    platformPolicy_->setLockdownMode(true);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    startConnecting();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kBlockedByOsPolicy);
    QCOMPARE(connectionFactory_->createdCount(), 0);

    // The error path must settle the state machine: a wedged Connecting would assert on the next
    // clickConnect and leave the reconnection cap timer running.
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testTunnelTestAttemptsZeroSurfacesResult()
{
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // ExtraConfig pins the tunnel-test attempt count to 0: the result is surfaced to the UI as-is
    // without failing over. Drive it through the tunnel-test seam rather than an on-disk config.
    ExtraConfigMock::hasTunnelTestAttempts = true;
    ExtraConfigMock::tunnelTestAttempts = 0;

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(false, "");

    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
}

void TestConnectionManager::testRequestUsernameForwarded()
{
    // The connector asking for a username is forwarded to the UI when no session-cached
    // credentials can answer it.
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QSignalSpy usernameSpy(cm_, &ConnectionManager::requestUsername);
    conn->driveUserInputRequired(UserInputType::Username);

    QTRY_COMPARE(usernameSpy.count(), 1);
}

void TestConnectionManager::testRequestPasswordForwarded()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QSignalSpy passwordSpy(cm_, &ConnectionManager::requestPassword);
    conn->driveUserInputRequired(UserInputType::Password);

    QTRY_COMPARE(passwordSpy.count(), 1);
}

void TestConnectionManager::testUsernameRequestAnsweredFromSessionCache()
{
    // Credentials the user supplied for a reconnect are cached for the session; when the fresh
    // connector asks for a username, the relay answers it directly instead of re-prompting the UI.
    startConnecting();
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
    startConnecting();
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
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

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
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QSignalSpy interfaceSpy(cm_, &ConnectionManager::interfaceUpdated);
    conn->driveInterfaceUpdated("wg0");

    QTRY_COMPARE(interfaceSpy.count(), 1);
    QCOMPARE(interfaceSpy.at(0).at(0).toString(), QString("wg0"));
}

void TestConnectionManager::testTeardownBeforeRecreate()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(conn->prepareCount(), 1);
    const int createdBefore = connectionFactory_->createdCount();

    conn->driveError(ConnectError::kTransientTunnelFailure);

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
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(conn->startConnectCount(), 1);

    // prepared() arriving in a state that does not expect a dial is dropped.
    conn->drivePrepared();
    QTest::qWait(20);
    QCOMPARE(conn->startConnectCount(), 1);

    // Sender guard: queue disconnected() and a stale prepared() back-to-back, then pump. The
    // disconnected handler retires this connector and creates + dials the replacement; the stale
    // prepared() from the retired sender is then delivered and must not dial the replacement again.
    conn->setAutoEmitDisconnected(false);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
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
    QPointer<FakeConnection> conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->prepareCount(), 1);
    QCOMPARE(conn->startConnectCount(), 0);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
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
    connectionFactory_->setPrepareFailure(ConnectError::kLocalProcessLaunchFailure);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    QPointer<FakeConnection> conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kLocalProcessLaunchFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->teardownTotal(), 1);
    QTest::qWait(10);
    QVERIFY(conn.isNull());
}

void TestConnectionManager::testPrepareFailedDuringUserDisconnectIgnored()
{
    // A prepareFailed already queued when the user clicks disconnect must not hard-stop with an
    // error; the queued disconnected() completes the user-click path instead.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    QPointer<FakeConnection> conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->prepareCount(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    conn->drivePrepareFailed(ConnectError::kLocalProcessLaunchFailure);
    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testUpdateConnectionSettingsPreDialIsNoOp()
{
    // Before the dial, a settings update must leave the in-flight attempt alone (pre-Stage 4 the
    // connector did not exist yet and the update early-returned; the dialed flag preserves that).
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testDnsAutoKeepsAdapterDnsAndEmptyOverride()
{
    // Auto DNS: the tunnel's own DNS stays on the adapter, the connector gets no override, and this
    // is the only mode that disables OS DoH.
    AdapterGatewayInfo info = vpnAdapterInfo();
    info.setDnsServers({types::IpAddress("10.255.255.1")});
    FakeConnection *conn = connectIkev2With(info);

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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

void TestConnectionManager::testAlwaysOnPlusDisabledMidSessionAllowsFetch()
{
    // The Always On+ gate reads the live flag, not a strategy-build snapshot: disabling the mode
    // mid-session (no strategy rebuild) must let the very next attempt fetch normally, leaving the
    // cached-config budget untouched.
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    connectionFactory_->setHasUsableStoredConfig(true);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    startConnecting();

    cm_->setFirewallAlwaysOnPlusEnabled(false);
    cm_->onHostnamesResolved();

    FakeConnection *conn = connectionFactory_->lastCreated();
    QCOMPARE(conn->prepareCount(), 1);
    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::Normal);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->cachedConfigAttempts(), 0);
}

void TestConnectionManager::testAlwaysOnPlusEnabledMidSessionUsesCachedConfig()
{
    // The strategy's availability is the raw stored-config fact, not fused with Always On+ at
    // build: enabling the mode mid-session (no strategy rebuild) must let the very next attempt
    // run from the cached config instead of aborting with kConfigFetchFailure.
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    connectionFactory_->setHasUsableStoredConfig(true);
    cm_->setFirewallAlwaysOnPlusEnabled(false);
    startConnecting();

    cm_->setFirewallAlwaysOnPlusEnabled(true);
    cm_->onHostnamesResolved();

    FakeConnection *conn = connectionFactory_->lastCreated();
    QCOMPARE(conn->prepareCount(), 1);
    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::CachedOnly);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->cachedConfigAttempts(), 1);
}

void TestConnectionManager::testAlwaysOnPlusCacheExhaustedAutomaticAdvances()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    attemptStrategyFactory_->setAutomaticMode(true);
    connectionFactory_->setHasUsableStoredConfig(true);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    startConnecting();
    // Exhaust the budget; the QCOMPARE guards the seeding chain (fake stored config -> real
    // recompute -> strategy), without which the test would silently exercise the unavailable arm.
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->exhaustCachedConfigBudget();
    QCOMPARE(strategy->cachedConfigAttempts(), IConnectionAttemptStrategy::kMaxCachedConfigAttempts);

    FakeConnection *conn = connectionFactory_->lastCreated();
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->onHostnamesResolved();

    // The budget is spent: let the strategy advance to a protocol that works without the API,
    // without ever preparing the connector.
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(conn->prepareCount(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
}

void TestConnectionManager::testAlwaysOnPlusCacheExhaustedManualAborts()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    attemptStrategyFactory_->setAutomaticMode(false);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    startConnecting();

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // No usable cached config at all; manual mode has no next protocol to advance to.
    cm_->onHostnamesResolved();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kConfigFetchFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
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
    QCOMPARE(attemptStrategyFactory_->lastCreated()->cachedConfigAttempts(), 0);
}

void TestConnectionManager::testDnsControldStartsCtrldAndUsesListenIp()
{
    // Control D DNS: prepare() starts ctrld with the single upstream, and the ctrld listen address
    // becomes both the adapter DNS override and the connector's override.
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());

    FakeConnection *conn = connectIkev2();

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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
    startConnecting();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kDnsServiceStartFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    // Stage 4 (early creation): the connector now exists before the DNS prep step runs, so the
    // abort leaves an idle connector behind instead of never creating one; it must not have dialed.
    QCOMPARE(connectionFactory_->createdCount(), 1);
    QCOMPARE(connectionFactory_->lastCreated()->startConnectCount(), 0);
}

void TestConnectionManager::testDnsCtrldKilledOnUserDisconnect()
{
    dnsConfigurator_->setConnectedDnsInfo(makeControldDnsInfo());

    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
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
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(ctrldManager_->runCount(), 1);
    const int killsBefore = ctrldManager_->killCount();

    conn->driveError(ConnectError::kTransientTunnelFailure);

    // The kill happens in the queued onConnectionDisconnected, after the state already flipped.
    QTRY_VERIFY(ctrldManager_->killCount() > killsBefore);
    QTRY_COMPARE(ctrldManager_->runCount(), 2);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
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

void TestConnectionManager::testWakeDuringErrorStoppingLetsErrorSurface()
{
    // Park in Stopping with a pending fatal error (connector still tearing down).
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    req.bEmitAuthError = true;
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);
    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);

    const int createdBefore = connectionFactory_->createdCount();
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);

    // A wake landing in this window must not convert the fatal error into a reconnect.
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    // The pending error still surfaces once the connector confirms it stopped.
    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kAuthFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testConnectedIgnoredDuringUserDisconnect()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    // Keep the connector alive through the disconnect so a late connected() can race in.
    conn->setAutoEmitDisconnected(false);

    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);

    // A connected() arriving mid-teardown must not resurrect the session.
    conn->driveConnected(vpnAdapterInfo());
    QTest::qWait(10);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testNetworkRecoveryFromWaitingReconnects()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Keep the connector alive so the parked state is observable across the offline/online cycle.
    conn->setAutoEmitDisconnected(false);

    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    const int disconnectsBefore = conn->startDisconnectCount();

    // Connectivity returning drives the parked session back into a reconnect.
    networkDetectionManager_->setOnline(true);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(cm_->timerReconnection_.isActive());
    QVERIFY(conn->startDisconnectCount() > disconnectsBefore);
}

void TestConnectionManager::testNetworkRecoveryAfterConnectorGoneReconnects()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Keep the connector alive so the disconnect confirmation can be driven after the park.
    conn->setAutoEmitDisconnected(false);

    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);

    // The connector confirms the disconnect while the network is still down: the online-state slot
    // ignores connectorless states, so the parked session must arm the connectivity poll instead.
    conn->driveDisconnected();
    QTRY_VERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->timerReconnection_.isActive());

    const int createdBefore = connectionFactory_->createdCount();
    networkDetectionManager_->setOnline(true);
    cm_->onTimerWaitNetworkConnectivity();
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), createdBefore + 1);

    // The redial must leave the park, or its own connected() is dropped as stale.
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    connectionFactory_->lastCreated()->driveConnected(vpnAdapterInfo());
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(connectedSpy.count(), 1);
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testWakeReconnectConvertsOnFirstFailure()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // Keep the connector alive so the wake reconnect parks on it instead of redialing immediately.
    conn->setAutoEmitDisconnected(false);

    // Wake initiates a reconnect: Reconnecting entered with the Wake cause, connector asked to stop.
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);

    // A connector error before the wake disconnect completes converts it to a regular reconnect:
    // reconnecting() is re-announced and the cause drops to Regular so failures now consume an endpoint.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_VERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Regular);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testConnectorErrorWhileStoppingIsInert()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);

    // The Stopping guard drops an error from the still-current connector: no failover, no endpoint consumed.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTest::qWait(10);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testSecondClickDisconnectWhileStoppingIsNoOp()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    // A repeat click while the user-initiated stop is in flight must not re-drive the connector.
    cm_->clickDisconnect();
    QCOMPARE(conn->startDisconnectCount(), 1);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QTest::qWait(10);
    QCOMPARE(disconnectedSpy.count(), 1);
}

void TestConnectionManager::testClickDisconnectDuringErrorStoppingConvertsOutcome()
{
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    req.bEmitAuthError = true;
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);

    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);

    // The user's click wins over the pending fatal error: the stop completes as a user disconnect.
    cm_->clickDisconnect();
    conn->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testUserDisconnectDuringProtocolRestWait()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    attemptStrategyFactory_->lastCreated()->setProtocolChanged(true);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_VERIFY(cm_->connectTimer_.isActive());
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // The rest wait has no connector, so the disconnect completes synchronously and kills the timer.
    cm_->clickDisconnect();

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QVERIFY(!cm_->connectTimer_.isActive());
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QTest::qWait(10);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
}

void TestConnectionManager::testBareDisconnectWhileConnectingStops()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // A regular strategy treats a bare process death mid-dial as attempt-fatal without consuming an endpoint.
    conn->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testWakeRedialAfterDisconnectCompletes()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
    QCOMPARE(conn->startDisconnectCount(), 1);
    const int createdBefore = connectionFactory_->createdCount();

    // The wake reconnect's disconnect completing converts the cause and redials with a fresh
    // reconnection budget, without consuming an endpoint.
    conn->driveDisconnected();

    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Regular);
    QVERIFY(cm_->timerReconnection_.isActive());
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testConnectorReconnectingWhileConnectingConsumesEndpoint()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    conn->driveReconnecting();

    QTRY_COMPARE(connectionFactory_->createdCount(), 2);
    QVERIFY(reconnectingSpy.count() >= 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(strategy->putFailedConnectionCount(), 1);
    FakeConnection *newConn = connectionFactory_->lastCreated();
    QTRY_COMPARE(newConn->startConnectCount(), 1);

    // Exhausted variant: the same signal with no endpoints left ends the session.
    strategy->setFailed(true);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    newConn->driveReconnecting();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(strategy->putFailedConnectionCount(), 2);
}

void TestConnectionManager::testDuplicateConnectorErrorsConsumeOneEndpoint()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);

    // Back-to-back failure signals before any redial: the ignore flag dedupes them to one endpoint.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    conn->driveReconnecting();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QTest::qWait(10);

    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
    QCOMPARE(conn->startDisconnectCount(), 1);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testOfflineTransitionsWhileDialed()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);

    // Offline mid-dial parks the attempt and starts the reconnection cap.
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), 1);

    // Recovery re-enters Reconnecting; a second outage parks it again.
    networkDetectionManager_->setOnline(true);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QCOMPARE(conn->startDisconnectCount(), 3);

    // A wake-initiated reconnect owns its own disconnect/redial: the offline arm leaves it alone.
    cm_->bLastIsOnline_ = true;
    const int disconnectsBefore = conn->startDisconnectCount();
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
    networkDetectionManager_->setOnline(true);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore + 1);
}

void TestConnectionManager::testOnlineChangePreDialIsNoOp()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    // The offline arms only make sense for a dialed attempt; a pre-dial park is left alone.
    networkDetectionManager_->setOnline(false);

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QCOMPARE(conn->startDisconnectCount(), 0);
    QVERIFY(!cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testReconnectionCapExpiryDuringRestWait()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    attemptStrategyFactory_->lastCreated()->setProtocolChanged(true);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_VERIFY(cm_->connectTimer_.isActive());
    QVERIFY(cm_->timerReconnection_.isActive());

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // The cap firing with the connector already retired must settle synchronously.
    cm_->timerReconnection_.stop();
    cm_->onTimerReconnection();

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->connectTimer_.isActive());
    QVERIFY(!cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testHostnamesResolvedNodeErrorFailsAttempt()
{
    // The strategy resolving to a broken descriptor is attempt-fatal: connector retired, error surfaced.
    CurrentConnectionDescr descr = makeIkev2Descr();
    descr.connectionNodeType = CONNECTION_NODE_ERROR;
    attemptStrategyFactory_->setCurrentConnectionSettings(descr);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    startConnecting();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kLocationUnavailable);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->teardownTotal(), 1);
    QTest::qWait(10);
    QVERIFY(connectionFactory_->lastCreated() == nullptr);
}

void TestConnectionManager::testLateHostnamesResolvedIgnored()
{
    connectIkev2();
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // A resolve completing after the attempt was abandoned has no connector to act on.
    cm_->onHostnamesResolved();

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testSleepDuringReconnecting()
{
    // Force the sleep-aware branch so the fake's waitForDisconnect completes the blocking stop.
    platformPolicy_->setNeedsSleepEventAwareDisconnect(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);

    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QCOMPARE(conn->teardownCount(), 1);

    // Wake restores the attempt from scratch.
    conn->setAutoEmitDisconnected(true);
    const int createdBefore = connectionFactory_->createdCount();
    sleepEvents_->driveWake();
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
}

void TestConnectionManager::testPrivKeyPasswordFlow()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QSignalSpy privKeySpy(cm_, &ConnectionManager::requestPrivKeyPassword);
    conn->driveUserInputRequired(UserInputType::PrivKeyPassword);
    QTRY_COMPARE(privKeySpy.count(), 1);

    cm_->continueWithPrivKeyPassword("pw", false);
    QCOMPARE(conn->continueWithUserInputCount(), 1);
    const auto *response = std::get_if<PrivKeyPasswordResponse>(&conn->lastUserInput());
    QVERIFY(response != nullptr);
    QCOMPARE(response->password, QString("pw"));

    // The reconnect flavor restarts the attempt instead of forwarding.
    const int createdBefore = connectionFactory_->createdCount();
    cm_->continueWithPrivKeyPassword("pw2", true);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QVERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testWakeDuringUserStoppingStaysStopped()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    const int createdBefore = connectionFactory_->createdCount();
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);

    // A wake mid user-disconnect must not hijack the stop into a wake reconnect.
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    // The user's disconnect still settles once the connector confirms it stopped.
    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
}

void TestConnectionManager::testReconnectionCapDuringUserStoppingIsInert()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // The reconnection cap firing mid-teardown must not overwrite the pending user outcome.
    cm_->onTimerReconnection();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(attemptStrategyFactory_->lastCreated()->resetCount() >= 1);
}

void TestConnectionManager::testLateConnectedDuringErrorStoppingIgnored()
{
    ConnectRequest req;
    req.bli = QSharedPointer<FakeLocationInfo>::create();
    req.bEmitAuthError = true;
    cm_->clickConnect(req);
    FakeConnection *conn = connectionFactory_->lastCreated();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);
    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // A connected() racing in during a non-user (error) stop must not resurrect the session; the
    // pending fatal error still surfaces once the connector confirms the stop.
    conn->driveConnected(vpnAdapterInfo());
    QTest::qWait(10);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kAuthFailure);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testConnectedQueuedBeforeSleepIgnoredInSleeping()
{
    platformPolicy_->setNeedsSleepEventAwareDisconnect(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    // Queue connected(), then sleep before it is delivered: the sleep-path blockingDisconnect pumps
    // no events and keeps the connector object alive, so the signal lands in kSleeping and must be
    // dropped rather than re-entering Connected against a torn-down tunnel.
    conn->driveConnected(vpnAdapterInfo());
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);

    QTest::qWait(20);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
}

void TestConnectionManager::testUserInputRequiredIgnoredAfterBlockingDisconnect()
{
    FakeConnection *conn = connectIkev2();

    QSignalSpy usernameSpy(cm_, &ConnectionManager::requestUsername);
    // blockingDisconnect keeps the connector object alive; a userInputRequired queued before it must
    // not raise a credential prompt after the attempt already settled.
    conn->driveUserInputRequired(UserInputType::Username);
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QTest::qWait(20);
    QCOMPARE(usernameSpy.count(), 0);
}

void TestConnectionManager::testStatsAndInterfaceIgnoredAfterBlockingDisconnect()
{
    FakeConnection *conn = connectIkev2();

    QSignalSpy statsSpy(cm_, &ConnectionManager::statisticsUpdated);
    QSignalSpy interfaceSpy(cm_, &ConnectionManager::interfaceUpdated);
    // Same retained-connector window: stats/interface updates queued before blockingDisconnect must
    // not feed Engine data for a tunnel that no longer exists.
    conn->driveStatisticsUpdated(1, 2, true);
    conn->driveInterfaceUpdated("wg0");
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QTest::qWait(20);
    QCOMPARE(statsSpy.count(), 0);
    QCOMPARE(interfaceSpy.count(), 0);
}

void TestConnectionManager::testUpdateConnectionSettingsWhileConnectingReconnects()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    // A settings change while an attempt is dialed (not yet connected) redials under the new settings.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(conn->startDisconnectCount(), 1);
    QVERIFY(reconnectingSpy.count() >= 1);
}

void TestConnectionManager::testConnectorErrorWhileWaitingForNetworkConsumesEndpoint()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);

    // Offline mid-dial parks the attempt in WaitingForNetwork with the connector retained.
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();

    // A queued connector error is not dropped in WaitingForNetwork (unlike kStopping/kSleeping): it
    // records the attempt failure and re-enters Reconnecting, which re-parks while still offline.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(strategy->putFailedConnectionCount(), 1);
}

void TestConnectionManager::testLateConnectedWhileWaitingForNetworkIgnored()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);

    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    // A connected() from the connector we just told to disconnect (offline) is stale: it must not
    // enter Connected from WaitingForNetwork.
    conn->driveConnected(vpnAdapterInfo());
    QTest::qWait(10);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
}

void TestConnectionManager::testOnlineChangeDuringStoppingIsInert()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    // A network flap mid user-stop must not re-drive the connector or disturb the pending outcome.
    networkDetectionManager_->setOnline(false);
    networkDetectionManager_->setOnline(true);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testBlockingDisconnectDuringRestWaitQuiesces()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    attemptStrategyFactory_->lastCreated()->setProtocolChanged(true);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_VERIFY(cm_->connectTimer_.isActive());
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);

    // In the protocol-rest wait the connector was retired (null) and connectTimer_ is armed.
    // blockingDisconnect must still settle to Disconnected and stop the rest timer, or it would fire
    // after the caller expected a full stop and redial.
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->connectTimer_.isActive());
}

void TestConnectionManager::testDualStackEgressSkipsGaiIpv4Priority()
{
    // A dual-stack tunnel carries IPv6 egress, so forcing gai.conf IPv4 priority would stall
    // IPv6-preferring apps; the connected path must skip it.
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->setDualStackEgress(true);
    conn->driveConnected(vpnAdapterInfo());
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kConnected);

    QCOMPARE(platformPolicy_->gaiIpv4PriorityCount(), 0);
}

void TestConnectionManager::testWaitNetworkPollGivesUpWhenCapExpired()
{
    // Parked with no default adapter, the connectivity poll gives up once the reconnection cap has
    // elapsed rather than polling forever.
    platformPolicy_->setDefaultAdapter(AdapterGatewayInfo());
    startConnecting();
    QVERIFY(cm_->timerWaitNetworkConnectivity_.isActive());

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // Force the cap to read as expired, then tick the poll while still adapterless.
    cm_->timerReconnection_.start(0);
    cm_->onTimerWaitNetworkConnectivity();

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
}

void TestConnectionManager::testSecondConnectTearsDownLeftoverConnector()
{
    // blockingDisconnect (Engine's normal disconnect) tears down but does not delete the connector.
    // The next clickConnect must retire that leftover so a stale event can't act on the new session.
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    const int teardownsBefore = connectionFactory_->teardownTotal();

    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    // The leftover connector (still alive after blockingDisconnect) was torn down again on the
    // second connect before the replacement was dialed.
    QCOMPARE(connectionFactory_->teardownTotal(), teardownsBefore + 1);
}

void TestConnectionManager::testReconnectingBareDisconnectExhaustedStops()
{
    // Endpoint-list strategy mid-reconnect: a bare process death with the list exhausted ends the
    // session instead of redialing the same endpoint forever.
    attemptStrategyFactory_->setRetryOnAttemptFailure(true);
    attemptStrategyFactory_->setFailed(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    // A spontaneous drop from Connected enters kReconnecting (Regular cause) and redials.
    conn->driveDisconnected();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Regular);
    FakeConnection *conn2 = connectionFactory_->lastCreated();
    QTRY_COMPARE(conn2->startConnectCount(), 1);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    const int createdBefore = connectionFactory_->createdCount();
    conn2->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
}

void TestConnectionManager::testReconnectionCapStopsWaitNetworkPoll()
{
    // The cap firing while parked adapterless must stop the connectivity poll, or a later network
    // return would redial from kDisconnected after the app reported it gave up.
    platformPolicy_->setDefaultAdapter(AdapterGatewayInfo());
    startConnecting();
    QVERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(connectionFactory_->createdCount(), 0);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // The cap fires with no connector dialed (still polling for network).
    cm_->timerReconnection_.stop();
    cm_->onTimerReconnection();

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());
}

void TestConnectionManager::testFatalErrorStopsConnectingTimer()
{
    // A fatal error entering kStopping must stop the connecting timer, or a timeout firing during
    // the connector teardown would convert the pending error into a failover reconnect.
    attemptStrategyFactory_->setAutomaticMode(false);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);
    QVERIFY(cm_->connectingTimer_.isActive());

    // Manual-mode tunnel failure is attempt-fatal: CM enters kStopping awaiting the connector's stop.
    // The error signal is queued, so let it settle.
    conn->driveError(ConnectError::kTunnelEstablishmentFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QVERIFY(!cm_->connectingTimer_.isActive());

    // The pending error still surfaces once the connector confirms the stop.
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    conn->driveDisconnected();
    QTRY_COMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kTunnelEstablishmentFailure);
}

void TestConnectionManager::testSleepFromWaitingForNetworkStopsConnectingTimer()
{
    // Sleeping from kWaitingForNetwork must stop the connecting timer, or an overdue timeout could
    // fire from kSleeping on resume and start a failover outside the wake path.
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);
    QVERIFY(cm_->connectingTimer_.isActive());

    // Offline mid-dial parks in kWaitingForNetwork with the connecting timer still armed.
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->connectingTimer_.isActive());

    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testDisconnectWhileWaitingAfterWakeArmsReconnectionCap()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    // Waking offline with a dialed connector parks with neither timer armed (the online-state slot
    // or the connector's own signals are expected to drive the recovery).
    cm_->bLastIsOnline_ = false;
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(!cm_->timerReconnection_.isActive());

    // The connector's late disconnect lands with the cap stopped: the poll must arm the cap too, or
    // the give-up check (remainingTime()==0, but -1 when inactive) can never fire and it polls forever.
    conn->driveDisconnected();

    QTRY_VERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QVERIFY(cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testFreshFetchAttemptResetsCachedConfigBudget()
{
    attemptStrategyFactory_->setCurrentConnectionSettings(makeWireGuardDescr());
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    connectionFactory_->setHasUsableStoredConfig(true);
    cm_->setFirewallAlwaysOnPlusEnabled(true);
    startConnecting();

    // Both cached attempts consumed, then the mode is disabled mid-walk: the counter is stale.
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    strategy->exhaustCachedConfigBudget();
    QCOMPARE(strategy->cachedConfigAttempts(), IConnectionAttemptStrategy::kMaxCachedConfigAttempts);
    cm_->setFirewallAlwaysOnPlusEnabled(false);

    // The next attempt fetches fresh, superseding the cached-attempt history.
    cm_->onHostnamesResolved();
    FakeConnection *conn = connectionFactory_->lastCreated();
    QCOMPARE(conn->prepareCount(), 1);
    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::Normal);
    QCOMPARE(strategy->cachedConfigAttempts(), 0);

    // A connect timeout of the fresh-fetch attempt says nothing about the cached config: the stale
    // exhausted counter must not wipe the config the fetch just stored.
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 0);
    QVERIFY(strategy->hasUsableCachedConfig());
}

void TestConnectionManager::testReconnectionCapExpiryDuringDialStopsConnectingTimer()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);
    QVERIFY(cm_->connectingTimer_.isActive());

    // The cap expiring mid-dial enters kStopping; the connect timeout must not survive into the
    // teardown, or its firing would convert the final stop into a failover reconnect.
    cm_->timerReconnection_.stop();
    cm_->onTimerReconnection();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QVERIFY(!cm_->connectingTimer_.isActive());

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testLateConnectedDuringWakeReconnectResurrects()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);

    QSignalSpy connectedSpy(cm_, &ConnectionManager::connected);
    // A late connected() during a wake reconnect (state kReconnecting) is honored: the tunnel came
    // back, so entering Connected is the right outcome. Only settled states (stopping/sleeping/
    // disconnected) drop it.
    conn->driveConnected(vpnAdapterInfo());

    QTRY_COMPARE(connectedSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QVERIFY(!cm_->timerReconnection_.isActive());
}

void TestConnectionManager::testConnectorReconnectingDuringWakeReconnectConsumesNoEndpoint()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    sleepEvents_->driveWake();
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);

    // A wake-initiated attempt's connector-level reconnect isn't a failure: no endpoint consumed,
    // the cause stays Wake.
    conn->driveReconnecting();
    QTest::qWait(10);

    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(cm_->reconnectCause_, ConnectionManager::ReconnectCause::Wake);
}

void TestConnectionManager::testAuthFailureWhileConnectingResumesWalkOnConnectorStop()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    conn->setAutoEmitDisconnected(false);

    // Without bEmitAuthError an auth failure is retryable, and the connector self-emits
    // disconnected() after it: CM must not force a second disconnect.
    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(conn->startDisconnectCount(), 0);

    // The walk resumes on the connector's own stop confirmation.
    const int createdBefore = connectionFactory_->createdCount();
    conn->driveDisconnected();
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
}

void TestConnectionManager::testCustomConfigAttemptSkipsConnectingTimeout()
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.protocol = types::Protocol::OPENVPN_UDP;
    d.ip = "10.0.0.4";
    d.port = 443;
    attemptStrategyFactory_->setCurrentConnectionSettings(d);
    attemptStrategyFactory_->setCustomConfig(true);

    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);

    // Custom-config endpoint walks have no connect timeout; arming it would abort every slow dial.
    QVERIFY(!cm_->connectingTimer_.isActive());
}

void TestConnectionManager::testAlwaysOnPlusCustomConfigWireGuardBypassesGate()
{
    // A custom WG config never fetches from the API, so the Always On+ cached-config gate must not
    // apply to it even though the connector has the cached-config capability.
    CurrentConnectionDescr d = makeWireGuardDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    attemptStrategyFactory_->setCurrentConnectionSettings(d);
    attemptStrategyFactory_->setCustomConfig(true);
    connectionFactory_->setHasUsableStoredConfig(true);
    cm_->setFirewallAlwaysOnPlusEnabled(true);

    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->prepareCount(), 1);

    QVERIFY(conn->lastPrepareEnv().configFetchMode == ConfigFetchMode::Normal);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->cachedConfigAttempts(), 0);
}

void TestConnectionManager::testWakeFromConnectorlessSleepRedials()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QTRY_COMPARE(conn->startConnectCount(), 1);
    attemptStrategyFactory_->lastCreated()->setProtocolChanged(true);

    // Park in the protocol rest wait: the failed connector is retired before the rest timer starts.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_VERIFY(cm_->connectTimer_.isActive());
    QVERIFY(cm_->connector_ == nullptr);

    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QVERIFY(!cm_->connectTimer_.isActive());

    // Waking with no connector must announce the reconnect and start a fresh attempt itself.
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    const int createdBefore = connectionFactory_->createdCount();
    cm_->bLastIsOnline_ = true;
    sleepEvents_->driveWake();

    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(reconnectingSpy.count() >= 1);
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
}

void TestConnectionManager::testDuplicatePreparedDialsOnce()
{
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->startConnectCount(), 0);

    // Overlapping resolves can deliver prepared() twice for one attempt; the second must not dial
    // the already-running connector again.
    conn->drivePrepared();
    conn->drivePrepared();
    QTRY_COMPARE(conn->startConnectCount(), 1);
    QTest::qWait(10);

    QCOMPARE(conn->startConnectCount(), 1);
}

void TestConnectionManager::testBlockingDisconnectFromConnectedRunsCleanup()
{
    FakeConnection *conn = connectIkev2();
    FakeConnectionAttemptStrategy *strategy = attemptStrategyFactory_->lastCreated();
    const int gaiBefore = platformPolicy_->gaiIpv4PriorityCount();
    const int killsBefore = ctrldManager_->killCount();

    cm_->blockingDisconnect(false);

    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(conn->teardownCount(), 1);
    QCOMPARE(platformPolicy_->gaiIpv4PriorityCount(), gaiBefore + 1);
    QVERIFY(!platformPolicy_->lastGaiIpv4Priority());
    QCOMPARE(ctrldManager_->killCount(), killsBefore + 1);
    QCOMPARE(strategy->resetCount(), 1);
}

void TestConnectionManager::testUpdateConnectionSettingsDuringReconnectingIsDeferred()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    conn->driveError(ConnectError::kTransientTunnelFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QPointer<FakeConnectionAttemptStrategy> oldStrategy = attemptStrategyFactory_->lastCreated();
    const int disconnectsBefore = conn->startDisconnectCount();

    // Mid-reconnect the settings change only swaps the strategy; the in-flight disconnect/redial
    // proceeds and the next attempt picks up the new settings.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QVERIFY(oldStrategy.isNull());
    QVERIFY(attemptStrategyFactory_->lastCreated() != nullptr);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore);
}

void TestConnectionManager::testEmptyWalkFailsWithLocationUnavailable()
{
    // A strategy whose walk filtered out every protocol has no dialable attempt; the connect must
    // fail cleanly instead of asking the factory for an uninitialized-protocol connector.
    attemptStrategyFactory_->setCurrentConnectionSettings(CurrentConnectionDescr());

    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    startConnecting();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(errorSpy.at(0).at(0)), ConnectError::kLocationUnavailable);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), 0);
}

void TestConnectionManager::testTunnelTestAttemptsZeroSuccessDropsIp()
{
    connectIkev2();
    ExtraConfigMock::hasTunnelTestAttempts = true;
    ExtraConfigMock::tunnelTestAttempts = 0;

    QSignalSpy testTunnelSpy(cm_, &ConnectionManager::testTunnelResult);
    cm_->onTunnelTestResult(true, "203.0.113.7");

    // The attempts-pinned-to-0 escape hatch surfaces the result as-is, without the IP.
    QCOMPARE(testTunnelSpy.count(), 1);
    QCOMPARE(testTunnelSpy.at(0).at(0).toBool(), true);
    QCOMPARE(testTunnelSpy.at(0).at(1).toString(), QString(""));
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
}

void TestConnectionManager::testErrorQueuedBeforeSleepIgnoredInSleeping()
{
    platformPolicy_->setNeedsSleepEventAwareDisconnect(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);

    // Queue the error, then sleep before it is delivered: the sleep-path blockingDisconnect pumps
    // no events and keeps the connector object, so the error lands in kSleeping.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);

    QTest::qWait(20);
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
}

void TestConnectionManager::testErrorAfterBlockingDisconnectIgnored()
{
    FakeConnection *conn = connectIkev2();

    // Same exposure in kDisconnected: blockingDisconnect keeps the connector object alive, so an
    // error queued before it can be delivered after the teardown settled.
    conn->driveError(ConnectError::kTransientTunnelFailure);
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QTest::qWait(20);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testClassifyPrepareErrorMatrix()
{
    QCOMPARE(classifyPrepareError(ConnectError::kLocalConfigGenerationFailure, true), PrepareErrorRouting::Failover);
    QCOMPARE(classifyPrepareError(ConnectError::kLocalConfigGenerationFailure, false), PrepareErrorRouting::Failover);
    QCOMPARE(classifyPrepareError(ConnectError::kConfigFetchFailure, true), PrepareErrorRouting::Failover);
    QCOMPARE(classifyPrepareError(ConnectError::kConfigFetchFailure, false), PrepareErrorRouting::HardStop);
    QCOMPARE(classifyPrepareError(ConnectError::kCustomConfigInvalid, true), PrepareErrorRouting::HardStop);
    QCOMPARE(classifyPrepareError(ConnectError::kCustomConfigInvalid, false), PrepareErrorRouting::HardStop);
}

void TestConnectionManager::testContinueWithPasswordForwards()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    cm_->continueWithPassword("pw");

    QCOMPARE(conn->continueWithUserInputCount(), 1);
    const auto *response = std::get_if<PasswordResponse>(&conn->lastUserInput());
    QVERIFY(response != nullptr);
    QCOMPARE(response->password, QString("pw"));
}

void TestConnectionManager::testOnlineChangeWhileConnectedReconnects()
{
    // macOS in production: a network switch fires the online slot with isAlive still true; from
    // Connected this forces a reconnect.
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    conn->setAutoEmitDisconnected(false);
    const int disconnectsBefore = conn->startDisconnectCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->onNetworkOnlineStateChanged(true);

    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QVERIFY(cm_->timerReconnection_.isActive());
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore + 1);
}

void TestConnectionManager::testProtocolPortChangedEmittedOnConnect()
{
    QSignalSpy protocolPortSpy(cm_, &ConnectionManager::protocolPortChanged);

    connectIkev2();

    QCOMPARE(protocolPortSpy.count(), 1);
    QCOMPARE(qvariant_cast<types::Protocol>(protocolPortSpy.at(0).at(0)), types::Protocol(types::Protocol::IKEV2));
    QCOMPARE(protocolPortSpy.at(0).at(1).toUInt(), 500u);
}

void TestConnectionManager::testConnectionEndedEmittedOnUserDisconnect()
{
    connectIkev2();
    QSignalSpy connectionEndedSpy(cm_, &ConnectionManager::connectionEnded);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);

    cm_->clickDisconnect();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(connectionEndedSpy.count(), 1);
}

void TestConnectionManager::testProtocolStatusForwardedAcrossStrategyRecreation()
{
    connectIkev2();
    QSignalSpy statusSpy(cm_, &ConnectionManager::protocolStatusChanged);

    attemptStrategyFactory_->lastCreated()->driveProtocolStatusChanged();
    QCOMPARE(statusSpy.count(), 1);

    // updateConnectionSettings replaces the strategy; the forwarding must be re-wired to the new one.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());
    attemptStrategyFactory_->lastCreated()->driveProtocolStatusChanged();
    QCOMPARE(statusSpy.count(), 2);
}

void TestConnectionManager::testLastConnectedIpAfterConnect()
{
    connectIkev2();

    // The dial snapshot records the connector's effective IP readback (the endpoint actually dialed).
    QCOMPARE(cm_->getLastConnectedIp(), QString("10.0.0.2"));
}

void TestConnectionManager::testAllowFirewallRuntimeNonCustomAlwaysTrue()
{
    FakeConnection *conn = connectIkev2();

    // Non-custom-config connections always allow the firewall, regardless of the connector's answer.
    conn->setAllowFirewallAfterConnectionRuntime(false);
    QVERIFY(cm_->isAllowFirewallAfterConnectionRuntime());
}

void TestConnectionManager::testAllowFirewallRuntimeCustomConfigDelegates()
{
    CurrentConnectionDescr descr = makeIkev2Descr();
    descr.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    attemptStrategyFactory_->setCurrentConnectionSettings(descr);
    attemptStrategyFactory_->setCustomConfig(true);

    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->driveConnected(vpnAdapterInfo());
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kConnected);

    conn->setAllowFirewallAfterConnectionRuntime(false);
    QVERIFY(!cm_->isAllowFirewallAfterConnectionRuntime());
    conn->setAllowFirewallAfterConnectionRuntime(true);
    QVERIFY(cm_->isAllowFirewallAfterConnectionRuntime());
}

void TestConnectionManager::testStaticIpsAccessors()
{
    CurrentConnectionDescr descr = makeStaticIpIkev2Descr();
    descr.staticIps.ports << 1195 << 1196;
    attemptStrategyFactory_->setCurrentConnectionSettings(descr);

    connectIkev2();

    QVERIFY(cm_->isStaticIpsLocation());
    const api_responses::StaticIpPortsVector ports = cm_->getStatisIps();
    QCOMPARE(ports.size(), 2);
    QCOMPARE(ports[0], 1195u);
    QCOMPARE(ports[1], 1196u);
}

void TestConnectionManager::testSetPacketSizePlumbedIntoPrepareEnv()
{
    types::PacketSize ps;
    ps.isAutomatic = false;
    ps.mtu = 1400;
    cm_->setPacketSize(ps);

    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QCOMPARE(conn->prepareCount(), 1);
    QCOMPARE(conn->lastPrepareEnv().packetSize.mtu, 1400);
    QVERIFY(!conn->lastPrepareEnv().packetSize.isAutomatic);
}

void TestConnectionManager::testRemoveIkev2ConnectionFromOSForwarded()
{
    cm_->removeIkev2ConnectionFromOS();

    QCOMPARE(connectionFactory_->removeIkev2Count(), 1);
}

void TestConnectionManager::testReconnectPreDialIsNoOp()
{
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    cm_->reconnect();

    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
    QCOMPARE(conn->startDisconnectCount(), 0);
}

void TestConnectionManager::testCurrentProtocolLifecycle()
{
    QCOMPARE(cm_->currentProtocol(), types::Protocol(types::Protocol::UNINITIALIZED));

    connectIkev2();

    QCOMPARE(cm_->currentProtocol(), types::Protocol(types::Protocol::IKEV2));
}

void TestConnectionManager::testUserDisconnectDuringWaitForNetwork()
{
    // Arm 1: parked offline with the dialed connector still alive -- the disconnect goes through
    // kStopping and the user reason survives to the connector's stop confirmation.
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());

    // Arm 2: parked with the connector gone and the connectivity poll running -- the disconnect
    // completes synchronously and must kill the poll, or it would redial from kDisconnected.
    networkDetectionManager_->setOnline(true);
    FakeConnection *conn2 = connectIkev2();
    conn2->setAutoEmitDisconnected(false);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    conn2->driveDisconnected();
    QTRY_VERIFY(cm_->timerWaitNetworkConnectivity_.isActive());
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);

    const int createdBefore = connectionFactory_->createdCount();
    cm_->clickDisconnect();
    QCOMPARE(disconnectedSpy.count(), 2);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(1).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());

    networkDetectionManager_->setOnline(true);
    QTest::qWait(10);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
}

void TestConnectionManager::testPollTickDuringStoppingIsInert()
{
    // Park a live undialed connector in kWaitingForNetwork with the connectivity poll running:
    // sleep mid-prepare keeps the connector object, and waking offline re-arms the poll.
    connectionFactory_->setPrepareBehavior(FakeConnection::PrepareBehavior::Manual);
    FakeConnection *conn = startConnecting();
    QCOMPARE(conn->startConnectCount(), 0);
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    cm_->bLastIsOnline_ = false;
    sleepEvents_->driveWake();
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QVERIFY(cm_->timerWaitNetworkConnectivity_.isActive());

    // The reconnection cap firing must stop the poll: a tick during kStopping would startAttempt(),
    // retiring the connector whose disconnected() carries the pending outcome and wedging the machine.
    conn->setAutoEmitDisconnected(false);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    const int createdBefore = connectionFactory_->createdCount();
    cm_->onTimerReconnection();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QVERIFY(!cm_->timerWaitNetworkConnectivity_.isActive());

    QTest::qWait(10);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    // The pending outcome still lands once the connector confirms it stopped.
    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)),
             DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testUpdateConnectionSettingsParkedStates()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    const int createdBefore = connectionFactory_->createdCount();
    const int disconnectsBefore = conn->startDisconnectCount();

    // Parked waiting for network: only the strategy swaps; the parked attempt is not redialed.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    // Mid-teardown: the settings change must not clobber the pending user outcome or redial.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testSleepDuringStoppingKeepsPendingOutcome()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    // Sleep mid user-disconnect must not hijack the stop with a blockingDisconnect (its teardown
    // would run here) and must leave the pending outcome to settle.
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->teardownCount(), 0);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testConnectionFactoryProtocolMap()
{
    // The real factory's protocol map with the per-family capabilities. All three connectors'
    // constructors are side-effect-free, so building them is safe in the harness.
    QObject parent;
    ConnectionFactory factory(helper_);
    ConnectRequest req;

    IConnection *wg = factory.createConnection(types::Protocol(types::Protocol::WIREGUARD), &parent, req);
    QVERIFY(wg != nullptr);
    QVERIFY(wg->capabilities().supportsCachedConfig);
    QVERIFY(wg->capabilities().needsSystemDnsRestore);

    IConnection *ovpn = factory.createConnection(types::Protocol(types::Protocol::OPENVPN_UDP), &parent, req);
    QVERIFY(ovpn != nullptr);
    QVERIFY(!ovpn->capabilities().supportsCachedConfig);
    QVERIFY(ovpn->capabilities().needsSystemDnsRestore);

    IConnection *ikev2 = factory.createConnection(types::Protocol(types::Protocol::IKEV2), &parent, req);
    QVERIFY(ikev2 != nullptr);
    QVERIFY(!ikev2->capabilities().supportsCachedConfig);
    QVERIFY(!ikev2->capabilities().needsSystemDnsRestore);
}

void TestConnectionManager::testRestoreSystemDnsOnTeardown()
{
    FakeHelperBackend *backend = static_cast<FakeHelperBackend *>(helper_->backend());
    const auto restoreCmdCount = [backend]() {
        return static_cast<int>(backend->sentCmdIds().count(static_cast<int>(HelperCommand::setDnsScriptEnabled)));
    };

    // A connector that changed no system DNS must not trigger the restore on teardown.
    connectIkev2();
    cm_->clickDisconnect();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(restoreCmdCount(), 0);

    // One that did (needsSystemDnsRestore) must: on macOS the restore reaches the helper as
    // setDnsScriptEnabled; on the other platforms restoreSystemDns is a no-op.
    FakeConnection *conn = connectIkev2();
    conn->setNeedsSystemDnsRestore(true);
    cm_->clickDisconnect();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
#ifdef Q_OS_MACOS
    QCOMPARE(restoreCmdCount(), 1);
#else
    QCOMPARE(restoreCmdCount(), 0);
#endif
}

void TestConnectionManager::testConnectingTimeoutExhaustedGivesUp()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);
    attemptStrategyFactory_->lastCreated()->setFailed(true);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->connectingTimer_.stop();
    cm_->onConnectingTimeout();

    // Pinned quirk: the failover announcement precedes the exhaustion check, so the last protocol's
    // timeout still emits reconnecting() before giving up.
    QCOMPARE(reconnectingSpy.count(), 1);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 1);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testAuthFailureExhaustedSettlesWithoutForcedDisconnect()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    conn->setAutoEmitDisconnected(false);
    attemptStrategyFactory_->lastCreated()->setFailed(true);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // Auth without bEmitAuthError is retryable but the exhausted walk gives up; auth is exempt from
    // the forced teardown, so the stop rides the connector's own disconnected().
    conn->driveError(ConnectError::kAuthFailure);
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(conn->startDisconnectCount(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_ITSELF);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testUnknownClassificationIsInert()
{
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    // A prepare-phase code misrouted through the runtime error signal has no classification and must
    // not hijack the live attempt.
    conn->driveError(ConnectError::kCustomConfigInvalid);

    QTest::qWait(10);
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnecting);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testBlockingDisconnectRestoresSystemDns()
{
    FakeHelperBackend *backend = static_cast<FakeHelperBackend *>(helper_->backend());
    const auto restoreCmdCount = [backend]() {
        return static_cast<int>(backend->sentCmdIds().count(static_cast<int>(HelperCommand::setDnsScriptEnabled)));
    };

    // The sleep/app-quit teardown has its own copy of the restore; a miss here leaves the OS
    // resolver pointing at the dead tunnel. macOS restores via setDnsScriptEnabled; elsewhere
    // restoreSystemDns is a no-op.
    FakeConnection *conn = connectIkev2();
    conn->setNeedsSystemDnsRestore(true);
    cm_->blockingDisconnect(false);

    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
#ifdef Q_OS_MACOS
    QCOMPARE(restoreCmdCount(), 1);
#else
    QCOMPARE(restoreCmdCount(), 0);
#endif
}

void TestConnectionManager::testUpdateConnectionSettingsBeforeConnectIsNoOp()
{
    // Engine pushes settings changes regardless of connection state; with no prior connect there is
    // no request to build a strategy from.
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());

    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(attemptStrategyFactory_->lastCreated() == nullptr);
}

void TestConnectionManager::testClickDisconnectDuringTimeoutStoppingConvertsOutcome()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->onTimerReconnection();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    // The user's click wins over the pending timeout outcome: the stop completes as a user
    // disconnect, including the user-stop strategy reset.
    cm_->clickDisconnect();
    conn->driveDisconnected();

    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QVERIFY(attemptStrategyFactory_->lastCreated()->resetCount() >= 1);
}

void TestConnectionManager::testReconnectingBareDisconnectWalksWhenNotExhausted()
{
    // Endpoint-list strategy mid-reconnect: a bare process death with the list NOT exhausted
    // consumes the endpoint and dials the next one instead of ending the session.
    attemptStrategyFactory_->setRetryOnAttemptFailure(true);
    FakeConnection *conn = connectIkev2();

    conn->driveDisconnected();
    QTRY_COMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    FakeConnection *conn2 = connectionFactory_->lastCreated();
    QTRY_COMPARE(conn2->startConnectCount(), 1);
    // The drop from Connected consumed nothing; only the mid-walk death below does.
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    conn2->driveDisconnected();

    QTRY_COMPARE(connectionFactory_->createdCount(), 3);
    QTRY_COMPARE(connectionFactory_->lastCreated()->startConnectCount(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(cm_->state_, ConnectionManager::State::kReconnecting);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 1);
}

void TestConnectionManager::testReconnectingSignalDuringStoppingIsInert()
{
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);

    // The connector is still current while the stop settles, so this reaches handleReconnecting()
    // without the error-slot state guard; it must not consume an endpoint or clobber the outcome.
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    const int createdBefore = connectionFactory_->createdCount();
    conn->driveReconnecting();
    QTest::qWait(10);
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);

    conn->driveDisconnected();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
}

void TestConnectionManager::testReconnectingSignalWhileWaitingForNetworkIsInert()
{
    platformPolicy_->setReconnectOnOnlineStateChange(true);
    FakeConnection *conn = connectIkev2();
    conn->setAutoEmitDisconnected(false);
    networkDetectionManager_->setOnline(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    const int createdBefore = connectionFactory_->createdCount();
    const int disconnectsBefore = conn->startDisconnectCount();
    conn->driveReconnecting();
    QTest::qWait(10);
    QCOMPARE(cm_->state_, ConnectionManager::State::kWaitingForNetwork);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(attemptStrategyFactory_->lastCreated()->putFailedConnectionCount(), 0);
}

void TestConnectionManager::testReconnectingFromConnectedWithSettledConnectorIsInert()
{
    FakeConnection *conn = connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);

    // A connector that already settled cannot be startDisconnect()ed into a reconnect cycle; its
    // own disconnected()/error() decides what happens next.
    conn->setAutoEmitDisconnected(false);
    conn->waitForDisconnect();

    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    const int disconnectsBefore = conn->startDisconnectCount();
    conn->driveReconnecting();
    QTest::qWait(10);

    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    QCOMPARE(conn->startDisconnectCount(), disconnectsBefore);
    QCOMPARE(reconnectingSpy.count(), 0);
}

void TestConnectionManager::testRemoveStoredConfigForwarded()
{
    cm_->removeStoredConfig();
    QCOMPARE(connectionFactory_->removeStoredConfigCount(), 1);
}

void TestConnectionManager::testUpdateConnectionSettingsWhileSleepingIsParked()
{
    connectIkev2();
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);

    // Only the strategy swaps; a redial here would dial mid-sleep.
    const int createdBefore = connectionFactory_->createdCount();
    cm_->updateConnectionSettings(types::ConnectionSettings(), api_responses::PortMap(), types::ProxySettings());
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QTest::qWait(10);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);
}

void TestConnectionManager::testSleepWakeWhileDisconnectedIsNoOp()
{
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    // A wake with no session must not start an attempt.
    sleepEvents_->driveWake();
    QTest::qWait(10);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(connectionFactory_->createdCount(), 0);
    QCOMPARE(disconnectedSpy.count(), 0);
}

void TestConnectionManager::testClickConnectClearsSessionCredentials()
{
    // Credentials cached for one session must not answer a later session's prompt: a stale answer
    // would silently auth-fail-loop instead of re-prompting the user.
    startConnecting();
    cm_->continueWithUsernameAndPassword("user1", "pass1", false);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QTRY_COMPARE(disconnectedSpy.count(), 1);

    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QSignalSpy usernameSpy(cm_, &ConnectionManager::requestUsername);
    conn->driveUserInputRequired(UserInputType::Username);

    QTRY_COMPARE(usernameSpy.count(), 1);
    QCOMPARE(conn->continueWithUserInputCount(), 0);
}

void TestConnectionManager::testDisconnectedQueuedBeforeSleepIgnoredInSleeping()
{
    platformPolicy_->setNeedsSleepEventAwareDisconnect(true);
    FakeConnection *conn = connectIkev2();

    // Queue the disconnected, then sleep before it is delivered: the sleep-path blockingDisconnect
    // pumps no events and keeps the connector object, so the event lands in kSleeping.
    conn->driveDisconnected();
    sleepEvents_->driveSleep();
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    const int createdBefore = connectionFactory_->createdCount();

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QTest::qWait(20);
    QCOMPARE(cm_->state_, ConnectionManager::State::kSleeping);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(connectionFactory_->createdCount(), createdBefore);

    // The stale event must not have corrupted the sleep session: wake still redials.
    cm_->bLastIsOnline_ = true;
    sleepEvents_->driveWake();
    QTRY_VERIFY(connectionFactory_->createdCount() > createdBefore);
}

void TestConnectionManager::testDisconnectedAfterBlockingDisconnectIgnored()
{
    FakeConnection *conn = connectIkev2();

    // Same exposure in kDisconnected: blockingDisconnect keeps the connector object alive, so a
    // disconnected queued before it can be delivered after the teardown settled.
    conn->driveDisconnected();
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    QSignalSpy reconnectingSpy(cm_, &ConnectionManager::reconnecting);
    QTest::qWait(20);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
    QCOMPARE(disconnectedSpy.count(), 0);
    QCOMPARE(reconnectingSpy.count(), 0);
    QCOMPARE(connectionFactory_->createdCount(), 1);
}

void TestConnectionManager::testConnectingTimerUsesConnectorTimeout()
{
    // Expiry is simulated by invoking the slot elsewhere, so the wiring of the interval to the
    // connector's capability must be pinned separately.
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);

    QVERIFY(cm_->connectingTimer_.isActive());
    QCOMPARE(cm_->connectingTimer_.interval(), conn->capabilities().connectTimeoutMs);
}

void TestConnectionManager::testStaleQueuedDisconnectedIgnoredAfterNewConnect()
{
    // A disconnected still queued from the previous session's connector must not be attributed to
    // the new attempt; the retired connector stays alive (deleteLater) so the sender check holds.
    connectIkev2();
    QCOMPARE(cm_->state_, ConnectionManager::State::kConnected);
    FakeConnection *oldConn = connectionFactory_->lastCreated();
    oldConn->driveDisconnected();
    cm_->blockingDisconnect(false);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);

    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn != oldConn);

    QTRY_COMPARE(conn->startConnectCount(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    QVERIFY(cm_->state_ != ConnectionManager::State::kDisconnected);
}

void TestConnectionManager::testHostnamesResolvedDuringStoppingIgnored()
{
    // A resolve landing while the stop is settling (connector alive, disconnected() still queued)
    // must not restart attempt setup mid-teardown.
    attemptStrategyFactory_->setAutoResolveHostnames(false);
    FakeConnection *conn = startConnecting();
    QVERIFY(conn);
    QCOMPARE(conn->prepareCount(), 0);

    QSignalSpy protocolPortSpy(cm_, &ConnectionManager::protocolPortChanged);
    QSignalSpy errorSpy(cm_, &ConnectionManager::errorDuringConnection);
    QSignalSpy disconnectedSpy(cm_, &ConnectionManager::disconnected);
    cm_->clickDisconnect();
    QCOMPARE(cm_->state_, ConnectionManager::State::kStopping);
    cm_->onHostnamesResolved();

    QCOMPARE(conn->prepareCount(), 0);
    QCOMPARE(protocolPortSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
    QTRY_COMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(qvariant_cast<DISCONNECT_REASON>(disconnectedSpy.at(0).at(0)), DISCONNECTED_BY_USER);
    QCOMPARE(cm_->state_, ConnectionManager::State::kDisconnected);
}

QTEST_MAIN(TestConnectionManager)
