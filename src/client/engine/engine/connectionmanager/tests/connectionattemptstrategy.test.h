#pragma once

#include <QObject>

class TestConnectionAttemptStrategy : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void testManualWireGuardOverrideSelectsNodeByIp();
    void testManualOverrideIgnoredForNonWireGuard();
    void testManualInvalidOverrideFallsBackToPreferredHostname();
    void testManualRetryAdvancesNode();
    void testManualRetryKeepsPreferredNode();
    void testManualRetryPreferredNodeMissingAdvances();
    void testAutoWireGuardOverrideSelectsNodeByIp();
    void testAutoRetryReadsOverrideLive();
    void testAutoWalkTwoAttemptsPerProtocolInOrder();
    void testAutoProxyEnabledSkipsOpenVpnUdpOnly();
    void testAutoSkipWireguardProtocol();
    void testAutoLockdownSkipsIkev2();
    void testAutoHasProtocolChangedOnEvenAttemptsOnly();
    void testAutoExhaustionAndReset();
    void testAutoDescrUsesFirstPortAndUseIpInd();
    void testAutoStaticIpsDescr();
    void testAutoProtocolStatusCadenceAndContents();
    void testManualFailsAfterTwoFailures();
    void testManualStaticIpsWireguardIpOverride();
    void testFactoryCustomConfigLocation();
    void testFactoryAutoAlwaysOnPlusSkipsWireguardWithoutCachedConfig();
    void testFactoryAutoAllProtocolsFilteredReportsErrorNode();
    void testFactoryManualWireguardSubstitutedToIkev2();
    void testFactoryManualWireguardSubstitutionSkipsIkev2UnderLockdown();
    void testFactoryManualWireguardFallsBackToOpenVpnUdp();
    void testFactoryManualWireguardUnchangedWhenNoAlternative();
    void testFactoryManualWireguardKeptWithCachedConfig();
    void testFactoryPropagatesCachedConfigAvailability();
    void testCustomConfigOvpnDescriptorAndNodeAdvance();
    void testCustomConfigWireGuardDescriptor();
    void testCustomConfigNoUsableRemoteReportsErrorNode();
    void testFactoryAutoAlwaysOnPlusKeepsWireguardWithCachedConfig();
    void testFactoryManualWireguardSubstitutionEmptyPortsGuard();
    void testAutoWalkSkipsEmptyPortsItem();
    void testManualStrategyResetClearsFailureCount();
    void testManualPreResolveProtocolMatchesSettings();
    void testDefaultPolicyVirtualsAutoAndManual();
    void testResolveHostnamesEmitsForAutoAndManual();
    void testCachedConfigBudgetEdges();
};
