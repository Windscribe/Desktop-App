#pragma once

#include <QObject>
#include <QTest>

#include "engine/adaptergatewayinfo.h"
#include "engine/connectionmanager/attemptstrategy/iconnectionattemptstrategy.h"
#include "engine/helper/helper.h"
#include "types/connecteddnsinfo.h"

class ConnectionManager;
class DnsConfigurator;
class FakeConnectionFactory;
class FakeConnectionAttemptStrategyFactory;
class FakeCtrldManager;
class FakeNetworkDetectionManager;
class FakePlatformPolicy;
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
    void testHappyPathConnect();
    void testTunnelTestPassAfterConnect();
    void testUserDisconnectFromConnected();
    void testUserDisconnectFromConnecting();
    void testUserDisconnectWhenNoConnector();
    void testConnectedIgnoredDuringUserDisconnect();
    void testFailoverRetriesWhenNotExhausted();
    void testFailoverExhaustedEmitsDisconnectedItself();
    void testErrorClassificationAuthErrorFatalWhenEmit();
    void testProcessNotRespondingSettlesViaForcedDisconnect();
    void testErrorClassificationImmediateStop();
    void testAuthErrorRetryableWhenNotEmit();
    void testManualModeWireGuardErrorIsFatal();
    void testAutomaticModeWireGuardErrorIsRetryable();
    void testWireGuardErrorClearsCachedWgConfigWhenExhausted();
    void testConnectionFactoryStoredConfigOps();
    void testClassifyConnectErrorMatrix();
    void testRecordFailureAndAdvance();
    void testReconnectionTimerExpiry();
    void testConnectingTimeout();
    void testConnectingTimeoutFiredByTimerFailsOver();
    void testConnectingTimeoutClearsCachedWgConfigWhenExhausted();
    void testConnectingTimeoutKeepsCachedWgConfig();
    void testNetworkOfflineWhileConnected();
    void testNetworkOfflineIgnoredWhenPolicyOptsOut();
    void testSleepWakeReconnect();
    void testWakeDuringBlockingDisconnect();
    void testWakeDuringErrorStoppingLetsErrorSurface();
    void testWakeReconnectConvertsOnFirstFailure();
    void testNetworkRecoveryFromWaitingReconnects();
    void testNetworkRecoveryAfterConnectorGoneReconnects();
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
    void testUpdateConnectionSettingsPreservesAttemptSucceeded();
    void testContinueWithUsernamePasswordForwards();
    void testContinueWithUsernamePasswordReconnect();
    void testSpontaneousDropFromConnectedReconnects();
    void testReconnectPublicMethod();
    void testTunnelTestFailedRetriesWhenNotExhausted();
    void testTunnelTestFailedExhaustedDisconnects();
    void testTunnelTestResultIgnoredWhenNotConnected();
    void testTunnelTestFailedCustomConfigEmitsResult();
    void testTunnelTestNoErrorFlagSurfacesResult();
    void testProtocolChangedStartsRestTimer();
    void testWakeWhileOfflineWithoutConnectorPollsAndReconnects();
    void testConnectorReconnectingSignalFromConnected();
    void testOnlineButNoDefaultAdapterWaitsForNetwork();
    void testNoWaitPolicyFailsFastWhenOffline();
    void testLockdownModeIkev2Error();
    void testTunnelTestAttemptsZeroSurfacesResult();
    void testRequestUsernameForwarded();
    void testRequestPasswordForwarded();
    void testUsernameRequestAnsweredFromSessionCache();
    void testPasswordRequestAnsweredFromSessionCache();
    void testStatisticsUpdatedForwarded();
    void testInterfaceUpdatedForwarded();
    void testTeardownBeforeRecreate();
    void testStalePreparedIgnored();
    void testDisconnectDuringPrepare();
    void testPrepareFailedRetiresConnector();
    void testPrepareFailedDuringUserDisconnectIgnored();
    void testUpdateConnectionSettingsPreDialIsNoOp();
    void testDnsAutoKeepsAdapterDnsAndEmptyOverride();
    void testDnsForcedBehavesAsAuto();
    void testDnsCustomIpv4OverridesAdapterDns();
    void testDnsLocalOverridesAdapterDns();
    void testWireGuardTunnelDnsListAuto();
    void testWireGuardTunnelDnsListLocal();
    void testWireGuardTunnelDnsListCustomIpv4();
    void testAlwaysOnPlusDisabledMidSessionAllowsFetch();
    void testAlwaysOnPlusEnabledMidSessionUsesCachedConfig();
    void testAlwaysOnPlusCacheExhaustedAutomaticAdvances();
    void testAlwaysOnPlusCacheExhaustedManualAborts();
    void testAlwaysOnPlusLeavesNonFetchingConnectorAlone();
    void testDnsControldStartsCtrldAndUsesListenIp();
    void testDnsCtrldStartFailureAbortsConnect();
    void testDnsCtrldKilledOnUserDisconnect();
    void testDnsCtrldKilledOnReconnect();
    void testDnsSplitDnsPassesUpstreamsAndHostnames();
    void testDnsDohUpstreamQueries();
    void testConnectorErrorWhileStoppingIsInert();
    void testSecondClickDisconnectWhileStoppingIsNoOp();
    void testClickDisconnectDuringErrorStoppingConvertsOutcome();
    void testUserDisconnectDuringProtocolRestWait();
    void testBareDisconnectWhileConnectingStops();
    void testWakeRedialAfterDisconnectCompletes();
    void testConnectorReconnectingWhileConnectingConsumesEndpoint();
    void testDuplicateConnectorErrorsConsumeOneEndpoint();
    void testOfflineTransitionsWhileDialed();
    void testOnlineChangePreDialIsNoOp();
    void testReconnectionCapExpiryDuringRestWait();
    void testHostnamesResolvedNodeErrorFailsAttempt();
    void testLateHostnamesResolvedIgnored();
    void testSleepDuringReconnecting();
    void testPrivKeyPasswordFlow();
    void testWakeDuringUserStoppingStaysStopped();
    void testLateConnectedDuringErrorStoppingIgnored();
    void testConnectedQueuedBeforeSleepIgnoredInSleeping();
    void testUserInputRequiredIgnoredAfterBlockingDisconnect();
    void testStatsAndInterfaceIgnoredAfterBlockingDisconnect();
    void testUpdateConnectionSettingsWhileConnectingReconnects();
    void testConnectorErrorWhileWaitingForNetworkConsumesEndpoint();
    void testLateConnectedWhileWaitingForNetworkIgnored();
    void testOnlineChangeDuringStoppingIsInert();
    void testBlockingDisconnectDuringRestWaitQuiesces();
    void testDualStackEgressSkipsGaiIpv4Priority();
    void testWaitNetworkPollGivesUpWhenCapExpired();
    void testSecondConnectTearsDownLeftoverConnector();
    void testReconnectingBareDisconnectExhaustedStops();
    void testReconnectionCapStopsWaitNetworkPoll();
    void testFatalErrorStopsConnectingTimer();
    void testSleepFromWaitingForNetworkStopsConnectingTimer();
    void testAttemptSucceededBankedOnlyInAutomaticMode();
    void testDisconnectWhileWaitingAfterWakeArmsReconnectionCap();
    void testFreshFetchAttemptResetsCachedConfigBudget();
    void testReconnectionCapExpiryDuringDialStopsConnectingTimer();
    void testLateConnectedDuringWakeReconnectResurrects();
    void testConnectorReconnectingDuringWakeReconnectConsumesNoEndpoint();
    void testAuthFailureWhileConnectingResumesWalkOnConnectorStop();
    void testCustomConfigAttemptSkipsConnectingTimeout();
    void testAlwaysOnPlusCustomConfigWireGuardBypassesGate();
    void testWakeFromConnectorlessSleepRedials();
    void testDuplicatePreparedDialsOnce();
    void testBlockingDisconnectFromConnectedRunsCleanup();
    void testUpdateConnectionSettingsDuringReconnectingIsDeferred();
    void testEmptyWalkFailsWithLocationUnavailable();
    void testTunnelTestAttemptsZeroSuccessDropsIp();
    void testErrorQueuedBeforeSleepIgnoredInSleeping();
    void testErrorAfterBlockingDisconnectIgnored();
    void testClassifyPrepareErrorMatrix();
    void testReconnectionCapDuringUserStoppingIsInert();
    void testContinueWithPasswordForwards();
    void testOnlineChangeWhileConnectedReconnects();
    void testProtocolPortChangedEmittedOnConnect();
    void testConnectionEndedEmittedOnUserDisconnect();
    void testProtocolStatusForwardedAcrossStrategyRecreation();
    void testLastConnectedIpAfterConnect();
    void testAllowFirewallRuntimeNonCustomAlwaysTrue();
    void testAllowFirewallRuntimeCustomConfigDelegates();
    void testStaticIpsAccessors();
    void testSetPacketSizePlumbedIntoPrepareEnv();
    void testRemoveIkev2ConnectionFromOSForwarded();
    void testReconnectPreDialIsNoOp();
    void testCurrentProtocolLifecycle();
    void testUserDisconnectDuringWaitForNetwork();
    void testPollTickDuringStoppingIsInert();
    void testUpdateConnectionSettingsParkedStates();
    void testSleepDuringStoppingKeepsPendingOutcome();
    void testConnectionFactoryProtocolMap();
    void testRestoreSystemDnsOnTeardown();
    void testConnectingTimeoutExhaustedGivesUp();
    void testAuthFailureExhaustedSettlesWithoutForcedDisconnect();
    void testUnknownClassificationIsInert();
    void testBlockingDisconnectRestoresSystemDns();
    void testUpdateConnectionSettingsBeforeConnectIsNoOp();
    void testClickDisconnectDuringTimeoutStoppingConvertsOutcome();
    void testReconnectingBareDisconnectWalksWhenNotExhausted();
    void testReconnectingSignalDuringStoppingIsInert();
    void testReconnectingSignalWhileWaitingForNetworkIsInert();
    void testReconnectingFromConnectedWithSettledConnectorIsInert();
    void testRemoveStoredConfigForwarded();
    void testUpdateConnectionSettingsWhileSleepingIsParked();
    void testSleepWakeWhileDisconnectedIsNoOp();
    void testClickConnectClearsSessionCredentials();
    void testDisconnectedQueuedBeforeSleepIgnoredInSleeping();
    void testDisconnectedAfterBlockingDisconnectIgnored();
    void testConnectingTimerUsesConnectorTimeout();
    void testStaleQueuedDisconnectedIgnoredAfterNewConnect();
    void testHostnamesResolvedDuringStoppingIgnored();

private:
    ConnectionManager *cm_ = nullptr;
    Helper *helper_ = nullptr;
    FakeNetworkDetectionManager *networkDetectionManager_ = nullptr;
    FakeConnectionFactory *connectionFactory_ = nullptr;
    FakePlatformPolicy *platformPolicy_ = nullptr;
    FakeConnectionAttemptStrategyFactory *attemptStrategyFactory_ = nullptr;
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

    // clickConnect with the strategy factory's current descriptor (IKEv2 default node unless a test
    // overrode it); returns the created connector still in the connecting state (not yet connected).
    FakeConnection *startConnecting();
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
