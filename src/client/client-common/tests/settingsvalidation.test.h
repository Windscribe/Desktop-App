#pragma once

#include <QObject>
#include <QTest>

class TestSettingsValidation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // EngineSettings
    void testEngineSettings_validNoop();
    void testEngineSettings_badLanguage();
    void testEngineSettings_badCustomOvpnPath();
    void testEngineSettings_oversizeNetworkPreferred();
    void testEngineSettings_outOfRangeDnsPolicy();

    // ProxySettings
    void testProxySettings_resetOnBadHost();
    void testProxySettings_resetOnBadPort();
    void testProxySettings_credentialOverflow();
    void testProxySettings_outOfRangeOption();

    // MacAddrSpoofing
    void testMacAddrSpoofing_badMacDisables();
    void testMacAddrSpoofing_badInterfaceDisables();

    // ConnectedDnsInfo
    void testConnectedDnsInfo_badUpstream1();
    void testConnectedDnsInfo_capHostnames();
    void testConnectedDnsInfo_filtersInvalidHostnames();

    // ConnectionSettings
    void testConnectionSettings_badPort();

    // FirewallSettings
    void testFirewallSettings_outOfRangeMode();

    // GuiSettings
    void testGuiSettings_outOfRangeAppSkin();
    void testShareSecureHotspot_oversizedSsidDisables();
    void testShareSecureHotspot_shortPasswordDisables();
    void testSplitTunneling_dropInvalidRoute();
    void testSplitTunneling_capRoutes();
    void testSplitTunneling_dropEmptyFullNameApp();

    // GuiPersistentState
    void testGuiPersistentState_validNoop();
    void testGuiPersistentState_badLastExternalIp();
    void testGuiPersistentState_capNetworkWhitelist();
    void testGuiPersistentState_capAppGeometry();

    // NetworkInterface
    void testNetworkInterface_badInterfaceNameCleared();

    // End-to-end forge round trip
    void testRoundTrip_engineSettings_forgedDnsPolicy();
};
