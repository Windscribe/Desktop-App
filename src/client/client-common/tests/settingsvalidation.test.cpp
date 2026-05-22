#include <QtTest>

#include "settingsvalidation.test.h"

#include <QSettings>

#include "types/connecteddnsinfo.h"
#include "types/connectionsettings.h"
#include "types/enginesettings.h"
#include "types/firewallsettings.h"
#include "types/guipersistentstate.h"
#include "types/guisettings.h"
#include "types/macaddrspoofing.h"
#include "types/networkinterface.h"
#include "types/proxysettings.h"
#include "types/sharesecurehotspot.h"
#include "types/splittunneling.h"
#include "utils/languagesutil.h"

void TestSettingsValidation::initTestCase()
{
    QCoreApplication::setOrganizationName("WindscribeTest");
    QCoreApplication::setApplicationName("settingsvalidation.test");
}

void TestSettingsValidation::cleanupTestCase()
{
    QSettings settings;
    settings.clear();
}

void TestSettingsValidation::testEngineSettings_validNoop()
{
    types::EngineSettingsData data;
    DNS_POLICY_TYPE origDnsPolicy = data.dnsPolicy;
    UPDATE_CHANNEL origUpdate = data.updateChannel;
    QString origLanguage = data.language;
    data.validate();
    QCOMPARE(data.dnsPolicy, origDnsPolicy);
    QCOMPARE(data.updateChannel, origUpdate);
    QCOMPARE(data.language, origLanguage);
}

void TestSettingsValidation::testEngineSettings_badLanguage()
{
    types::EngineSettingsData data;
    data.language = "xx-XX-not-a-language";
    data.validate();
    QCOMPARE(data.language, LanguagesUtil::systemLanguage());
}

void TestSettingsValidation::testEngineSettings_badCustomOvpnPath()
{
    types::EngineSettingsData data;
    data.customOvpnConfigsPath = "/this/path/does/not/exist/foo";
    data.validate();
    QVERIFY(data.customOvpnConfigsPath.isEmpty());
}

void TestSettingsValidation::testEngineSettings_oversizeNetworkPreferred()
{
    types::EngineSettingsData data;
    for (int i = 0; i < 300; ++i) {
        data.networkPreferredProtocols.insert(QString("network_%1").arg(i), types::ConnectionSettings());
    }
    data.validate();
    QVERIFY(data.networkPreferredProtocols.size() <= 256);
}

void TestSettingsValidation::testEngineSettings_outOfRangeDnsPolicy()
{
    types::EngineSettingsData data;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    data.dnsPolicy = static_cast<DNS_POLICY_TYPE>(999);
    data.validate();
    QCOMPARE(data.dnsPolicy, DNS_TYPE_CLOUDFLARE);
}

void TestSettingsValidation::testProxySettings_resetOnBadHost()
{
    types::ProxySettings ps;
    ps.setOption(PROXY_OPTION_HTTP);
    ps.setAddress("abc;rm -rf");
    ps.setPort(8080);
    ps.validate();
    QCOMPARE(ps.option(), PROXY_OPTION_NONE);
    QVERIFY(ps.address().isEmpty());
}

void TestSettingsValidation::testProxySettings_resetOnBadPort()
{
    types::ProxySettings ps;
    ps.setOption(PROXY_OPTION_HTTP);
    ps.setAddress("1.2.3.4");
    ps.setPort(0);
    ps.validate();
    QCOMPARE(ps.option(), PROXY_OPTION_NONE);
}

void TestSettingsValidation::testProxySettings_credentialOverflow()
{
    types::ProxySettings ps;
    ps.setOption(PROXY_OPTION_HTTP);
    ps.setAddress("1.2.3.4");
    ps.setPort(8080);
    ps.setPassword(QString(300, 'x'));
    ps.validate();
    QCOMPARE(ps.option(), PROXY_OPTION_NONE);
    QVERIFY(ps.getPassword().isEmpty());
}

void TestSettingsValidation::testProxySettings_outOfRangeOption()
{
    types::ProxySettings ps;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    ps.setOption(static_cast<PROXY_OPTION>(999));
    ps.validate();
    QCOMPARE(ps.option(), PROXY_OPTION_NONE);
}

void TestSettingsValidation::testMacAddrSpoofing_badMacDisables()
{
    types::MacAddrSpoofing mac;
    mac.isEnabled = true;
    mac.macAddress = "not-a-mac";
    mac.validate();
    QVERIFY(mac.macAddress.isEmpty());
    QVERIFY(!mac.isEnabled);
}

void TestSettingsValidation::testMacAddrSpoofing_badInterfaceDisables()
{
    types::MacAddrSpoofing mac;
    mac.isEnabled = true;
    mac.selectedNetworkInterface.interfaceName = "bad\tiface\tname";
    mac.validate();
    QVERIFY(mac.selectedNetworkInterface.interfaceName.isEmpty());
    QVERIFY(!mac.isEnabled);
}

void TestSettingsValidation::testConnectedDnsInfo_badUpstream1()
{
    types::ConnectedDnsInfo dns;
    dns.type = CONNECTED_DNS_TYPE_CUSTOM;
    dns.upStream1 = "abc;rm -rf";
    dns.validate();
    QVERIFY(dns.upStream1.isEmpty());
    QCOMPARE(dns.type, CONNECTED_DNS_TYPE_AUTO);
}

void TestSettingsValidation::testConnectedDnsInfo_capHostnames()
{
    types::ConnectedDnsInfo dns;
    QStringList hostnames;
    for (int i = 0; i < 2000; ++i) {
        hostnames << QString("host%1.example.com").arg(i);
    }
    dns.hostnames = hostnames;
    dns.validate();
    QVERIFY(dns.hostnames.size() <= 1024);
}

void TestSettingsValidation::testConnectedDnsInfo_filtersInvalidHostnames()
{
    types::ConnectedDnsInfo dns;
    dns.hostnames = QStringList{"valid.example.com", "abc;rm -rf", "1.2.3.4", "another bad one"};
    dns.validate();
    QVERIFY(dns.hostnames.contains("valid.example.com"));
    QVERIFY(dns.hostnames.contains("1.2.3.4"));
    QVERIFY(!dns.hostnames.contains("abc;rm -rf"));
    QVERIFY(!dns.hostnames.contains("another bad one"));
}

void TestSettingsValidation::testConnectionSettings_badPort()
{
    types::ConnectionSettings cs;
    cs.setPort(70000);
    cs.validate();
    QVERIFY(cs.port() < 65536);
}

void TestSettingsValidation::testFirewallSettings_outOfRangeMode()
{
    types::FirewallSettings fw;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    fw.mode = static_cast<FIREWALL_MODE>(999);
    fw.validate();
    QCOMPARE(fw.mode, FIREWALL_MODE_AUTOMATIC);
}

void TestSettingsValidation::testGuiSettings_outOfRangeAppSkin()
{
    types::GuiSettings gs;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    gs.appSkin = static_cast<APP_SKIN>(999);
    gs.validate();
    QCOMPARE(gs.appSkin, APP_SKIN_ALPHA);
}

void TestSettingsValidation::testShareSecureHotspot_oversizedSsidDisables()
{
    types::ShareSecureHotspot sh;
    sh.isEnabled = true;
    sh.ssid = QString(64, 'a');
    sh.password = "validpassword";
    sh.validate();
    QVERIFY(sh.ssid.isEmpty());
    QVERIFY(!sh.isEnabled);
}

void TestSettingsValidation::testShareSecureHotspot_shortPasswordDisables()
{
    types::ShareSecureHotspot sh;
    sh.isEnabled = true;
    sh.ssid = "MyNetwork";
    sh.password = "abc";
    sh.validate();
    QVERIFY(sh.password.isEmpty());
    QVERIFY(!sh.isEnabled);
}

void TestSettingsValidation::testSplitTunneling_dropInvalidRoute()
{
    types::SplitTunneling st;
    types::SplitTunnelingNetworkRoute good;
    good.name = "10.0.0.0/8";
    good.type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    good.active = true;
    types::SplitTunnelingNetworkRoute bad;
    bad.name = "abc;rm -rf";
    bad.type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    bad.active = true;
    st.networkRoutes << good << bad;
    st.validate();
    QCOMPARE(st.networkRoutes.size(), 1);
    QCOMPARE(st.networkRoutes[0].name, QString("10.0.0.0/8"));
}

void TestSettingsValidation::testSplitTunneling_capRoutes()
{
    types::SplitTunneling st;
    for (int i = 0; i < 2000; ++i) {
        types::SplitTunnelingNetworkRoute r;
        r.name = QString("10.0.%1.0/24").arg(i % 256);
        r.type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
        r.active = true;
        st.networkRoutes << r;
    }
    st.validate();
    QVERIFY(st.networkRoutes.size() <= 1024);
}

void TestSettingsValidation::testSplitTunneling_dropEmptyFullNameApp()
{
    types::SplitTunneling st;
    types::SplitTunnelingApp good;
    good.fullName = "/Applications/Foo.app";
    good.name = "Foo";
    types::SplitTunnelingApp empty;
    empty.fullName = "";
    empty.name = "Missing";
    st.apps << good << empty;
    st.validate();
    QCOMPARE(st.apps.size(), 1);
    QCOMPARE(st.apps[0].fullName, QString("/Applications/Foo.app"));
}

void TestSettingsValidation::testGuiPersistentState_validNoop()
{
    types::GuiPersistentState s;
    QString origIp = s.lastExternalIp;
    LOCATION_TAB origTab = s.lastLocationTab;
    s.validate();
    QCOMPARE(s.lastExternalIp, origIp);
    QCOMPARE(s.lastLocationTab, origTab);
}

void TestSettingsValidation::testGuiPersistentState_badLastExternalIp()
{
    types::GuiPersistentState s;
    s.lastExternalIp = "999.999.999.999";
    s.validate();
    QCOMPARE(s.lastExternalIp, QString("---.---.---.---"));
}

void TestSettingsValidation::testGuiPersistentState_capNetworkWhitelist()
{
    types::GuiPersistentState s;
    for (int i = 0; i < 500; ++i) {
        types::NetworkInterface nif;
        nif.networkOrSsid = QString("net_%1").arg(i);
        s.networkWhiteList << nif;
    }
    s.validate();
    QVERIFY(s.networkWhiteList.size() <= 256);
}

void TestSettingsValidation::testGuiPersistentState_capAppGeometry()
{
    types::GuiPersistentState s;
    s.appGeometry = QByteArray(200 * 1024, 'x');
    s.validate();
    QVERIFY(s.appGeometry.isEmpty());
}

void TestSettingsValidation::testNetworkInterface_badInterfaceNameCleared()
{
    types::NetworkInterface nif;
    nif.interfaceName = "bad\tname";
    nif.friendlyName = "preserved";
    nif.validate();
    QVERIFY(nif.interfaceName.isEmpty());
    QCOMPARE(nif.friendlyName, QString("preserved"));
}

void TestSettingsValidation::testRoundTrip_engineSettings_forgedDnsPolicy()
{
    // Forge a value out of range, then exercise validate() — same code path as load.
    types::EngineSettingsData data;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    data.dnsPolicy = static_cast<DNS_POLICY_TYPE>(999);
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    data.dnsManager = static_cast<DNS_MANAGER_TYPE>(123);
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    data.updateChannel = static_cast<UPDATE_CHANNEL>(50);
    data.validate();
    QCOMPARE(data.dnsPolicy, DNS_TYPE_CLOUDFLARE);
    QCOMPARE(data.dnsManager, DNS_MANAGER_AUTOMATIC);
    QCOMPARE(data.updateChannel, UPDATE_CHANNEL_RELEASE);
}

QTEST_MAIN(TestSettingsValidation)
