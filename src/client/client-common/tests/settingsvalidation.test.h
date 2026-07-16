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
    void testProxySettings_resetOnDomain();
    void testProxySettings_resetOnBadPort();
    void testProxySettings_credentialOverflow();
    void testProxySettings_outOfRangeOption();
    void testProxySettings_clearedWhenOptionNone();
    void testProxySettings_clearedWhenOptionSocks();

    // SoundSettings
    void testSoundSettings_clearsPathWhenNone();
    void testSoundSettings_resetOnNonResourceBundledPath();
    void testSoundSettings_resetOnBadCustomPath();
    void testSoundSettings_outOfRangeType();

    // BackgroundSettings
    void testBackgroundSettings_clearsPathWhenUnused();
    void testBackgroundSettings_resetOnNonResourceBundledPath();
    void testBackgroundSettings_resetOnBadCustomPath();
    void testBackgroundSettings_resetsAspectRatioWhenUnused();

    // PacketSize
    void testPacketSize_resetOnBadMtu();
    void testPacketSize_validNoop();

    // MacAddrSpoofing
    void testMacAddrSpoofing_badMacDisables();
    void testMacAddrSpoofing_badInterfaceDisables();

    // ConnectedDnsInfo
    void testConnectedDnsInfo_badUpstream1();
    void testConnectedDnsInfo_capHostnames();
    void testConnectedDnsInfo_filtersInvalidHostnames();
    void testConnectedDnsInfo_normalizesUnspecifiedUpstream();
    void testConnectedDnsInfo_oversizeControldApiKey();
    void testConnectedDnsInfo_clearsFieldsWhenAuto();
    void testConnectedDnsInfo_clearsUnusedFieldsWhenCustom();

    // ConnectionSettings
    void testConnectionSettings_badPort();

    // FirewallSettings
    void testFirewallSettings_outOfRangeMode();
    void testFirewallSettings_resetsWhenWhenNotAutomatic();

    // GuiSettings
    void testGuiSettings_outOfRangeAppSkin();
    void testShareSecureHotspot_oversizedSsidDisables();
    void testShareSecureHotspot_shortPasswordDisables();
    void testSplitTunneling_dropInvalidRoute();
    void testSplitTunneling_capApps();
    void testSplitTunneling_capHostnames();
    void testSplitTunneling_capIpRoutes();
    void testSplitTunneling_mixedRouteCapsKeepIpRoutes();
    void testSplitTunneling_dropEmptyFullNameApp();
    void testSplitTunneling_rederivesRouteType();

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
