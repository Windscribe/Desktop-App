#pragma once

#include <QObject>
#include <QTest>

#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/helper/helper.h"

class ConnectionManager;
class CustomOvpnAuthCredentialsStorage;
class FakeConnectionFactory;
class FakeConnSettingsPolicyFactory;
class FakeNetworkDetectionManager;
class FakePlatformPolicy;
class FakeExtraConfig;
class FakeSleepEvents;
class FakeConnection;

// Characterization tests: they pin ConnectionManager's current observable behavior (emitted signals +
// state_) so the Stage 3-6 refactor can be verified behavior-preserving. TestConnectionManager is a
// friend of ConnectionManager, so tests read state_/timers and invoke private slots directly where a
// real signal path can't be reproduced with fakes.
class TestConnectionManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstructAndDestructWithFakes();

    // Happy path + user disconnect.
    void testHappyPathConnect();
    void testHappyPathConnectStaticIpNode();
    void testTunnelTestPassAfterConnect();
    void testUserDisconnectFromConnected();
    void testUserDisconnectFromConnecting();
    void testUserDisconnectWhenNoConnector();

    // Failover + error classification.
    void testFailoverRetriesWhenNotExhausted();
    void testFailoverExhaustedEmitsDisconnectedItself();
    void testErrorClassificationAuthErrorFatalWhenEmit();
    void testErrorClassificationImmediateStop();
    void testAuthErrorRetryableWhenNotEmit();
    void testManualModeWireGuardErrorIsFatal();
    void testAutomaticModeWireGuardErrorIsRetryable();

    // Timers + network + sleep/wake.
    void testReconnectionTimerExpiry();
    void testConnectingTimeout();
    void testNetworkOfflineWhileConnected();
    void testSleepWakeReconnect();
    void testWakeDuringBlockingDisconnect();

    // WireGuard key limit / config fetch, settings update, custom-config auth.
    void testWireGuardKeyLimitPrompt();
    void testWireGuardKeyLimitDecline();
    void testWireGuardConfigFetchFailedManual();
    void testWireGuardConfigFetchFailedAutomatic();
    void testUpdateConnectionSettingsWhileConnectedReconnects();
    void testContinueWithUsernamePasswordForwards();
    void testContinueWithUsernamePasswordReconnect();

    // Additional state-machine paths.
    void testSpontaneousDropFromConnectedReconnects();
    void testReconnectPublicMethod();
    void testTunnelTestFailedRetriesWhenNotExhausted();
    void testTunnelTestFailedExhaustedDisconnects();
    void testTunnelTestFailedCustomConfigEmitsResult();
    void testTunnelTestNoErrorFlagSurfacesResult();
    void testProtocolChangedStartsRestTimer();
    void testWakeWhileOfflineWaitsForNetwork();
    void testConnectorReconnectingSignalFromConnected();
    void testOnlineButNoDefaultAdapterWaitsForNetwork();
    void testLockdownModeIkev2Error();
    void testTunnelTestAttemptsZeroSurfacesResult();

    // Connector -> UI signal forwarding.
    void testRequestUsernameForwarded();
    void testRequestPasswordForwarded();
    void testStatisticsUpdatedForwarded();
    void testInterfaceUpdatedForwarded();

private:
    ConnectionManager *cm_ = nullptr;
    Helper *helper_ = nullptr;
    FakeNetworkDetectionManager *networkDetectionManager_ = nullptr;
    CustomOvpnAuthCredentialsStorage *credentialsStorage_ = nullptr;
    FakeConnectionFactory *connectionFactory_ = nullptr;
    FakePlatformPolicy *platformPolicy_ = nullptr;
    FakeExtraConfig *extraConfig_ = nullptr;
    FakeConnSettingsPolicyFactory *connSettingsPolicyFactory_ = nullptr;
    FakeSleepEvents *sleepEvents_ = nullptr;

    static AdapterGatewayInfo vpnAdapterInfo();
    static CurrentConnectionDescr makeIkev2Descr();
    static CurrentConnectionDescr makeWireGuardDescr();
    static CurrentConnectionDescr makeStaticIpIkev2Descr();

    // clickConnect an IKEv2 default-node session; returns the created connector still in the
    // connecting state (not yet connected).
    FakeConnection *startConnectingIkev2();
    // As above, then drive connected() and pump the queued signal.
    FakeConnection *connectIkev2();
};
