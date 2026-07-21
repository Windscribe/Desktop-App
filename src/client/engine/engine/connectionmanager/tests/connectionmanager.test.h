#pragma once

#include <QObject>
#include <QTest>

#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/connectionmanager/connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/helper/helper.h"
#include "types/connecteddnsinfo.h"

class ConnectionManager;
class CustomOvpnAuthCredentialsStorage;
class DnsConfigurator;
class FakeConnectionFactory;
class FakeConnSettingsPolicyFactory;
class FakeCtrldManager;
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
    void testWireGuardErrorClearsCachedWgConfigWhenExhausted();

    // Timers + network + sleep/wake.
    void testReconnectionTimerExpiry();
    void testConnectingTimeout();
    void testConnectingTimeoutClearsCachedWgConfigWhenExhausted();
    void testConnectingTimeoutKeepsCachedWgConfig();
    void testNetworkOfflineWhileConnected();
    void testSleepWakeReconnect();
    void testWakeDuringBlockingDisconnect();

    // WireGuard key limit / config fetch, settings update, custom-config auth.
    void testWireGuardKeyLimitPrompt();
    void testWireGuardKeyLimitDecline();
    void testWireGuardKeyLimitConsentResumesConnector();
    void testStaleConnectorSignalsIgnored();
    void testSleepDuringPrepareQuiescesAttempt();
    void testWireGuardKeyLimitIgnoredDuringDisconnect();
    void testWireGuardKeyLimitAnswerIgnoredWhenDisconnected();
    void testRetryPolicyWalksOnBareDisconnect();
    void testRetryPolicyBareDisconnectExhaustedStops();
    void testRetryPolicyWalksOnFatalError();
    void testBlockingDisconnectQuiescesPreDialAttempt();
    void testWireGuardConfigFetchFailedManual();
    void testWireGuardConfigFetchFailedAutomatic();
    void testConfigFetchFailedFailsOverInManualMode();
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
    void testNoWaitPolicyFailsFastWhenOffline();
    void testLockdownModeIkev2Error();
    void testTunnelTestAttemptsZeroSurfacesResult();

    // Connector -> UI signal forwarding.
    void testRequestUsernameForwarded();
    void testRequestPasswordForwarded();
    void testUsernameRequestAnsweredFromSessionCache();
    void testPasswordRequestAnsweredFromSessionCache();
    void testStatisticsUpdatedForwarded();
    void testInterfaceUpdatedForwarded();

    // Connector prepare/teardown lifecycle (Stage 4 contract).
    void testTeardownBeforeRecreate();
    void testStalePreparedIgnored();
    void testDisconnectDuringPrepare();
    void testPrepareFailedRetiresConnector();
    void testPrepareFailedDuringUserDisconnectIgnored();
    void testUpdateConnectionSettingsPreDialIsNoOp();

    // Connected-DNS behavior.
    void testDnsAutoKeepsAdapterDnsAndEmptyOverride();
    void testDnsForcedBehavesAsAuto();
    void testDnsCustomIpv4OverridesAdapterDns();
    void testDnsLocalOverridesAdapterDns();
    void testWireGuardTunnelDnsListAuto();
    void testWireGuardTunnelDnsListLocal();
    void testWireGuardTunnelDnsListCustomIpv4();

    // Always On+ cached-config gate (keyed on the supportsCachedConfig capability).
    void testAlwaysOnPlusCachedConfigFetchMode();
    void testAlwaysOnPlusCacheExhaustedAutomaticAdvances();
    void testAlwaysOnPlusCacheExhaustedManualAborts();
    void testAlwaysOnPlusLeavesNonFetchingConnectorAlone();

    // Connected-DNS ctrld branches (fake ctrld manager).
    void testDnsControldStartsCtrldAndUsesListenIp();
    void testDnsCtrldStartFailureAbortsConnect();
    void testDnsCtrldKilledOnUserDisconnect();
    void testDnsCtrldKilledOnReconnect();
    void testDnsSplitDnsPassesUpstreamsAndHostnames();
    void testDnsDohUpstreamQueries();

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
    // The real DnsConfigurator runs over fakes so the DNS characterization tests exercise the
    // production DNS logic end-to-end; dnsPlatformPolicy_ is its own policy instance, separate
    // from the one injected into ConnectionManager.
    DnsConfigurator *dnsConfigurator_ = nullptr;
    FakePlatformPolicy *dnsPlatformPolicy_ = nullptr;
    FakeCtrldManager *ctrldManager_ = nullptr;

    static AdapterGatewayInfo vpnAdapterInfo();
    static CurrentConnectionDescr makeIkev2Descr();
    static CurrentConnectionDescr makeWireGuardDescr();
    static CurrentConnectionDescr makeStaticIpIkev2Descr();
    static types::ConnectedDnsInfo makeDnsInfo(CONNECTED_DNS_TYPE type, const QString &upstream = QString());
    static types::ConnectedDnsInfo makeControldDnsInfo();

    // clickConnect an IKEv2 default-node session; returns the created connector still in the
    // connecting state (not yet connected).
    FakeConnection *startConnectingIkev2();
    // As above, then drive connected() and pump the queued signal.
    FakeConnection *connectIkev2();
    // As connectIkev2, but the connector reports the given adapter info in connected().
    FakeConnection *connectIkev2With(const AdapterGatewayInfo &info);
    // clickConnect a WireGuard default node and wait for the dial; returns the created connector.
    FakeConnection *connectWireGuard();
    // clickConnect a WireGuard default node whose prepare() requests the key-limit consent; returns
    // the created connector parked on the prompt.
    FakeConnection *startConnectingWireGuardAtKeyLimit();
};
