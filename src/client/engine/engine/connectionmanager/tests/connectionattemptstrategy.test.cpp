#include "connectionattemptstrategy.test.h"

#include <QtTest>

#include "engine/connectionmanager/attemptstrategy/autoconnectionattemptstrategy.h"
#include "engine/connectionmanager/attemptstrategy/connectionattemptstrategyfactory.h"
#include "engine/connectionmanager/attemptstrategy/customconfigconnectionattemptstrategy.h"
#include "engine/connectionmanager/attemptstrategy/manualconnectionattemptstrategy.h"
#include "engine/locationsmodel/customconfiglocationinfo.h"
#include "engine/locationsmodel/mutablelocationinfo.h"
#include "engine/locationsmodel/tests/customconfigfixtures.h"
#include "extraconfig_mock.h"
#include "types/locationid.h"
#include "types/proxysettings.h"

using customconfig_fixtures::makeOvpnCustomConfigLocationInfo;
using customconfig_fixtures::makeWireGuardCustomConfigLocationInfo;

namespace {

QSharedPointer<locationsmodel::MutableLocationInfo> makeLocationInfo()
{
    QVector<QSharedPointer<const locationsmodel::BaseNode>> nodes;
    nodes << QSharedPointer<const locationsmodel::BaseNode>(
        new locationsmodel::ApiLocationNode({"10.0.0.1", "10.0.0.1", "10.0.0.1"}, "node1", 1, "pubkey1", false));
    nodes << QSharedPointer<const locationsmodel::BaseNode>(
        new locationsmodel::ApiLocationNode({"10.0.0.2", "10.0.0.2", "10.0.0.2"}, "node2", 1, "pubkey2", false));
    nodes << QSharedPointer<const locationsmodel::BaseNode>(
        new locationsmodel::ApiLocationNode({"10.0.0.3", "10.0.0.3", "10.0.0.3"}, "node3", 1, "pubkey3", false));
    return QSharedPointer<locationsmodel::MutableLocationInfo>::create(
        LocationID::createApiLocationId(1, "City", "Nick"), "Location", nodes, 0, "dns.host", "");
}

api_responses::PortMap makePortMap(const QVector<types::Protocol> &protocols)
{
    api_responses::PortMap portMap;
    for (const auto &protocol : protocols) {
        api_responses::PortItem item;
        item.protocol = protocol;
        item.heading = protocol.toLongString();
        item.use = "ip";
        item.ports = {443};
        portMap.items() << item;
    }
    return portMap;
}

QSharedPointer<locationsmodel::MutableLocationInfo> makeStaticIpsLocationInfo()
{
    api_responses::StaticIpPortsVector staticPorts;
    staticPorts << 1195 << 1196;
    QVector<QSharedPointer<const locationsmodel::BaseNode>> nodes;
    nodes << QSharedPointer<const locationsmodel::BaseNode>(
        new locationsmodel::StaticLocationNode({"10.1.0.1", "10.1.0.2", "10.1.0.3"}, "static1", "pubkey1", "10.255.255.1",
                                               "dns.static", "user1", "pass1", staticPorts, false));
    return QSharedPointer<locationsmodel::MutableLocationInfo>::create(
        LocationID::createStaticIpsLocationId("City", "10.1.0.1"), "Static", nodes, 0, "dns.host", "");
}

} // namespace

void TestConnectionAttemptStrategy::initTestCase()
{
    qRegisterMetaType<QVector<types::ProtocolStatus>>("QVector<types::ProtocolStatus>");
}

void TestConnectionAttemptStrategy::init()
{
    ExtraConfigMock::reset();
}

void TestConnectionAttemptStrategy::testManualWireGuardOverrideSelectsNodeByIp()
{
    auto li = makeLocationInfo();
    ExtraConfigMock::remoteIp = "10.0.0.2";

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "");

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));
}

void TestConnectionAttemptStrategy::testManualOverrideIgnoredForNonWireGuard()
{
    auto li = makeLocationInfo();
    ExtraConfigMock::remoteIp = "10.0.0.2";

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::OPENVPN_UDP, 443, false),
                                             makePortMap({types::Protocol::OPENVPN_UDP}), "");

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node1"));
}

void TestConnectionAttemptStrategy::testManualInvalidOverrideFallsBackToPreferredHostname()
{
    auto li = makeLocationInfo();
    ExtraConfigMock::remoteIp = "not-an-ip";

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "node3");

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node3"));
}

void TestConnectionAttemptStrategy::testManualRetryAdvancesNode()
{
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "");
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node1"));

    strategy.putFailedConnection();

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));
}

void TestConnectionAttemptStrategy::testManualRetryKeepsPreferredNode()
{
    // With a preferred node set, a retry re-selects it rather than advancing to the next node.
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "node2");
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));

    strategy.putFailedConnection();

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));
}

void TestConnectionAttemptStrategy::testManualRetryPreferredNodeMissingAdvances()
{
    // A preferred node absent from the location leaves the default node initially, and a retry
    // falls through to the next node instead of wedging on the missing one.
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "ghost");
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node1"));

    strategy.putFailedConnection();

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));
}

void TestConnectionAttemptStrategy::testAutoWireGuardOverrideSelectsNodeByIp()
{
    auto li = makeLocationInfo();
    ExtraConfigMock::remoteIp = "10.0.0.2";

    AutoConnectionAttemptStrategy strategy(li, makePortMap({types::Protocol::WIREGUARD}), false, false, false, "");

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));
}

void TestConnectionAttemptStrategy::testAutoRetryReadsOverrideLive()
{
    auto li = makeLocationInfo();

    AutoConnectionAttemptStrategy strategy(li, makePortMap({types::Protocol::WIREGUARD}), false, false, false, "");
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node1"));

    // The override is read live on every attempt, so a value appearing mid-walk applies to the
    // next node change.
    ExtraConfigMock::remoteIp = "10.0.0.3";
    strategy.putFailedConnection();

    QCOMPARE(li->getHostnameForSelectedNode(), QString("node3"));
}

void TestConnectionAttemptStrategy::testAutoWalkTwoAttemptsPerProtocolInOrder()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
    portMap.items()[1].ports = {500};
    portMap.items()[2].ports = {1194};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, false, "");

    const QVector<QPair<types::Protocol, uint>> expected = {
        {types::Protocol::WIREGUARD, 443u}, {types::Protocol::WIREGUARD, 443u},
        {types::Protocol::IKEV2, 500u}, {types::Protocol::IKEV2, 500u},
        {types::Protocol::OPENVPN_UDP, 1194u}, {types::Protocol::OPENVPN_UDP, 1194u}};
    for (const auto &attempt : expected) {
        QVERIFY(!strategy.isFailed());
        const CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
        QCOMPARE(descr.protocol, attempt.first);
        QCOMPARE(descr.port, attempt.second);
        strategy.putFailedConnection();
    }
    QVERIFY(strategy.isFailed());
}

void TestConnectionAttemptStrategy::testAutoProxyEnabledSkipsOpenVpnUdpOnly()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::OPENVPN_UDP, types::Protocol::STUNNEL, types::Protocol::WIREGUARD});

    AutoConnectionAttemptStrategy strategy(li, portMap, true, false, false, "");

    const QVector<types::Protocol> expected = {
        types::Protocol::STUNNEL, types::Protocol::STUNNEL, types::Protocol::WIREGUARD, types::Protocol::WIREGUARD};
    for (const auto &protocol : expected) {
        QVERIFY(!strategy.isFailed());
        QCOMPARE(strategy.getCurrentConnectionSettings().protocol, protocol);
        strategy.putFailedConnection();
    }
    QVERIFY(strategy.isFailed());
}

void TestConnectionAttemptStrategy::testAutoSkipWireguardProtocol()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2});

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, true, "");

    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::IKEV2));
    strategy.putFailedConnection();
    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::IKEV2));
    strategy.putFailedConnection();
    QVERIFY(strategy.isFailed());
}

void TestConnectionAttemptStrategy::testAutoLockdownSkipsIkev2()
{
    // Under macOS Lockdown Mode, IKEv2 is dropped from the automatic walk (NEVPNManager is blocked).
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
    portMap.items()[2].ports = {1194};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, true, false, "");

    const QVector<types::Protocol> expected = {
        types::Protocol::WIREGUARD, types::Protocol::WIREGUARD,
        types::Protocol::OPENVPN_UDP, types::Protocol::OPENVPN_UDP};
    for (const auto &protocol : expected) {
        QVERIFY(!strategy.isFailed());
        QCOMPARE(strategy.getCurrentConnectionSettings().protocol, protocol);
        strategy.putFailedConnection();
    }
    QVERIFY(strategy.isFailed());
}

void TestConnectionAttemptStrategy::testAutoHasProtocolChangedOnEvenAttemptsOnly()
{
    auto li = makeLocationInfo();

    AutoConnectionAttemptStrategy strategy(li, makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2}), false, false, false, "");

    QVERIFY(strategy.hasProtocolChanged());
    strategy.putFailedConnection();
    QVERIFY(!strategy.hasProtocolChanged());
    strategy.putFailedConnection();
    QVERIFY(strategy.hasProtocolChanged());
    strategy.putFailedConnection();
    QVERIFY(!strategy.hasProtocolChanged());
}

void TestConnectionAttemptStrategy::testAutoExhaustionAndReset()
{
    auto li = makeLocationInfo();

    AutoConnectionAttemptStrategy strategy(li, makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2}), false, false, false, "");

    for (int i = 0; i < 4; ++i) {
        strategy.putFailedConnection();
    }
    QVERIFY(strategy.isFailed());

    strategy.reset();

    QVERIFY(!strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    QVERIFY(strategy.hasProtocolChanged());
    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
}

void TestConnectionAttemptStrategy::testAutoDescrUsesFirstPortAndUseIpInd()
{
    QVector<QSharedPointer<const locationsmodel::BaseNode>> nodes;
    nodes << QSharedPointer<const locationsmodel::BaseNode>(
        new locationsmodel::ApiLocationNode({"10.0.0.1", "10.0.1.1", "10.0.2.1"}, "node1", 1, "pubkey1", true));
    auto li = QSharedPointer<locationsmodel::MutableLocationInfo>::create(
        LocationID::createApiLocationId(1, "City", "Nick"), "Location", nodes, 0, "dns.host", "x509name");
    auto portMap = makePortMap({types::Protocol::WIREGUARD});
    portMap.items()[0].use = "ip2";
    portMap.items()[0].ports = {1194, 443};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, false, "");

    const CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_DEFAULT);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(descr.port, 1194u);
    QCOMPARE(descr.ip, QString("10.0.1.1"));
    QCOMPARE(descr.hostname, QString("node1"));
    QCOMPARE(descr.wireGuard.peerPublicKey, QString("pubkey1"));
    QCOMPARE(descr.verifyX509name, QString("x509name"));
    QVERIFY(descr.isIpv6Support);
}

void TestConnectionAttemptStrategy::testAutoStaticIpsDescr()
{
    auto li = makeStaticIpsLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2});
    portMap.items()[1].ports = {500};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, false, "");

    CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_STATIC_IPS);
    QCOMPARE(descr.staticIps.credentials.username(), QString("user1"));
    QCOMPARE(descr.staticIps.credentials.password(), QString("pass1"));
    QCOMPARE(descr.staticIps.ports.size(), 2);
    QCOMPARE(descr.staticIps.ports[0], 1195u);
    QCOMPARE(descr.staticIps.ports[1], 1196u);
    // WireGuard attempts on a static-IPs location connect to the dedicated wg_ip, not the node ip.
    QCOMPARE(descr.ip, QString("10.255.255.1"));

    strategy.putFailedConnection();
    strategy.putFailedConnection();

    descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::IKEV2));
    QCOMPARE(descr.port, 500u);
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_STATIC_IPS);
    QCOMPARE(descr.ip, QString("10.1.0.1"));
}

void TestConnectionAttemptStrategy::testAutoProtocolStatusCadenceAndContents()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
    portMap.items()[1].ports = {500};
    portMap.items()[2].ports = {1194};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, false, "");
    QSignalSpy spy(&strategy, &IConnectionAttemptStrategy::protocolStatusChanged);

    // Odd attempt indices retry the same protocol, so no status change is broadcast.
    strategy.putFailedConnection();
    QCOMPARE(spy.count(), 0);

    strategy.putFailedConnection();
    QCOMPARE(spy.count(), 1);
    auto status = spy.at(0).at(0).value<QVector<types::ProtocolStatus>>();
    QCOMPARE(spy.at(0).at(1).toBool(), true);
    QCOMPARE(status.size(), 3);
    QCOMPARE(status[0].protocol, types::Protocol(types::Protocol::IKEV2));
    QCOMPARE(status[0].port, 500);
    QCOMPARE(status[0].status, types::ProtocolStatus::kUpNext);
    QCOMPARE(status[1].protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QCOMPARE(status[1].status, types::ProtocolStatus::kDisconnected);
    QCOMPARE(status[2].protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(status[2].status, types::ProtocolStatus::kFailed);

    strategy.putFailedConnection();
    QCOMPARE(spy.count(), 1);
    strategy.putFailedConnection();
    QCOMPARE(spy.count(), 2);
    strategy.putFailedConnection();
    QCOMPARE(spy.count(), 2);

    strategy.putFailedConnection();
    QVERIFY(strategy.isFailed());
    QCOMPARE(spy.count(), 3);
    status = spy.at(2).at(0).value<QVector<types::ProtocolStatus>>();
    QCOMPARE(status.size(), 3);
    for (const auto &s : status) {
        QCOMPARE(s.status, types::ProtocolStatus::kFailed);
    }
}

void TestConnectionAttemptStrategy::testManualFailsAfterTwoFailures()
{
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::IKEV2, 443, false),
                                             makePortMap({types::Protocol::IKEV2}), "");
    QSignalSpy spy(&strategy, &IConnectionAttemptStrategy::protocolStatusChanged);
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node1"));

    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    QCOMPARE(spy.count(), 0);
    QCOMPARE(li->getHostnameForSelectedNode(), QString("node2"));

    strategy.putFailedConnection();
    QVERIFY(strategy.isFailed());
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.at(0).at(0).value<QVector<types::ProtocolStatus>>().isEmpty());
    QCOMPARE(spy.at(0).at(1).toBool(), false);
}

void TestConnectionAttemptStrategy::testManualStaticIpsWireguardIpOverride()
{
    auto li = makeStaticIpsLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                             makePortMap({types::Protocol::WIREGUARD}), "");

    const CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_STATIC_IPS);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(descr.port, 443u);
    QCOMPARE(descr.ip, QString("10.255.255.1"));
    QCOMPARE(descr.staticIps.credentials.username(), QString("user1"));
    QCOMPARE(descr.staticIps.credentials.password(), QString("pass1"));
}

void TestConnectionAttemptStrategy::testFactoryCustomConfigLocation()
{
    auto li = makeOvpnCustomConfigLocationInfo();
    QVERIFY(!li.isNull());

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::OPENVPN_UDP, 443, false),
                               makePortMap({types::Protocol::OPENVPN_UDP}), types::ProxySettings(), "", false, true, false));

    QVERIFY(!strategy->usesConnectTimeout());
    QVERIFY(strategy->surfacesTunnelTestFailure());
    QVERIFY(!strategy->isAutomaticMode());
    // The custom-config strategy never consults the cached-config advice; the factory must not seed it.
    QVERIFY(!strategy->hasUsableCachedConfig());
}

void TestConnectionAttemptStrategy::testFactoryAutoAlwaysOnPlusSkipsWireguardWithoutCachedConfig()
{
    auto li = makeLocationInfo();

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, true),
                               makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2}), types::ProxySettings(), "", true, false, false));

    QVERIFY(strategy->isAutomaticMode());
    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::IKEV2));
    strategy->putFailedConnection();
    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::IKEV2));
    strategy->putFailedConnection();
    // Only the two IKEv2 attempts existed: WireGuard never entered the walk.
    QVERIFY(strategy->isFailed());
}

void TestConnectionAttemptStrategy::testFactoryAutoAllProtocolsFilteredReportsErrorNode()
{
    auto li = makeLocationInfo();

    // Always On+ without a cached config filters WireGuard, leaving a WireGuard-only portmap with no
    // attempts at all: the empty walk must report an error node (ConnectionManager fails the attempt
    // with kLocationUnavailable) instead of indexing an empty list.
    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, true),
                               makePortMap({types::Protocol::WIREGUARD}), types::ProxySettings(), "", true, false, false));

    QCOMPARE(strategy->getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
    strategy->putFailedConnection();
    QVERIFY(strategy->isFailed());
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardSubstitutedToIkev2()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
    portMap.items()[1].ports = {500};

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               portMap, types::ProxySettings(), "", true, false, false));

    QVERIFY(!strategy->isAutomaticMode());
    const CurrentConnectionDescr descr = strategy->getCurrentConnectionSettings();
#ifdef Q_OS_LINUX
    // IKEv2 is unsupported on Linux; the substitution falls through to OpenVPN UDP.
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QCOMPARE(descr.port, 443u);
#else
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::IKEV2));
    QCOMPARE(descr.port, 500u);
#endif
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardSubstitutionSkipsIkev2UnderLockdown()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
    portMap.items()[1].ports = {500};
    portMap.items()[2].ports = {1194};

    // A substituted IKEv2 would hard-fail the lockdown gate at attempt time; the substitution must
    // fall through to OpenVPN UDP even with IKEv2 present in the portmap.
    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               portMap, types::ProxySettings(), "", true, false, true));

    const CurrentConnectionDescr descr = strategy->getCurrentConnectionSettings();
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QCOMPARE(descr.port, 1194u);
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardFallsBackToOpenVpnUdp()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::OPENVPN_UDP});
    portMap.items()[1].ports = {1194};

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               portMap, types::ProxySettings(), "", true, false, false));

    const CurrentConnectionDescr descr = strategy->getCurrentConnectionSettings();
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QCOMPARE(descr.port, 1194u);
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardUnchangedWhenNoAlternative()
{
    auto li = makeLocationInfo();

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               makePortMap({types::Protocol::WIREGUARD}), types::ProxySettings(), "", true, false, false));

    const CurrentConnectionDescr descr = strategy->getCurrentConnectionSettings();
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(descr.port, 443u);
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardKeptWithCachedConfig()
{
    auto li = makeLocationInfo();

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2}), types::ProxySettings(), "", true, true, false));

    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    QVERIFY(strategy->hasUsableCachedConfig());
}

void TestConnectionAttemptStrategy::testFactoryPropagatesCachedConfigAvailability()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD});

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> withConfig(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, true),
                               portMap, types::ProxySettings(), "", false, true, false));
    QScopedPointer<IConnectionAttemptStrategy> withoutConfig(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, true),
                               portMap, types::ProxySettings(), "", false, false, false));

    QVERIFY(withConfig->hasUsableCachedConfig());
    QVERIFY(!withoutConfig->hasUsableCachedConfig());
}

void TestConnectionAttemptStrategy::testCustomConfigOvpnDescriptorAndNodeAdvance()
{
    auto li = makeOvpnCustomConfigLocationInfo("dev tun\nproto udp\nauth-user-pass\nremote 1.2.3.4 443\nremote 5.6.7.8 1194\n");
    QVERIFY(!li.isNull());

    CustomConfigConnectionAttemptStrategy strategy(li);
    QCOMPARE(strategy.preResolveProtocol(), types::Protocol(types::Protocol::OPENVPN_UDP));

    QSignalSpy resolvedSpy(&strategy, &IConnectionAttemptStrategy::hostnamesResolved);
    strategy.resolveHostnames();
    QCOMPARE(resolvedSpy.count(), 1);

    CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_CUSTOM_CONFIG);
    QCOMPARE(descr.ip, QString("1.2.3.4"));
    QCOMPARE(descr.port, 443u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QVERIFY(descr.openVpn.customConfig.contains("dev tun"));
    QVERIFY(descr.openVpn.customConfig.contains("remote 1.2.3.4 443"));
    QVERIFY(descr.wireGuard.customConfig.isNull());

    // The walk is endless: a failure advances to the next remote and wraps, never exhausting.
    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.ip, QString("5.6.7.8"));
    QCOMPARE(descr.port, 1194u);
    QVERIFY(descr.openVpn.customConfig.contains("remote 5.6.7.8 1194"));

    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().ip, QString("1.2.3.4"));
}

void TestConnectionAttemptStrategy::testCustomConfigWireGuardDescriptor()
{
    auto li = makeWireGuardCustomConfigLocationInfo();
    QVERIFY(!li.isNull());

    CustomConfigConnectionAttemptStrategy strategy(li);
    QCOMPARE(strategy.preResolveProtocol(), types::Protocol(types::Protocol::WIREGUARD));

    QSignalSpy resolvedSpy(&strategy, &IConnectionAttemptStrategy::hostnamesResolved);
    strategy.resolveHostnames();
    QCOMPARE(resolvedSpy.count(), 1);

    const CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_CUSTOM_CONFIG);
    QCOMPARE(descr.ip, QString("1.2.3.4"));
    QCOMPARE(descr.port, 51820u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::WIREGUARD));
    QVERIFY(!descr.wireGuard.customConfig.isNull());
    QCOMPARE(descr.wireGuard.customConfig->clientDnsAddress(), QString("10.255.255.1"));
    QVERIFY(descr.openVpn.customConfig.isEmpty());
}

void TestConnectionAttemptStrategy::testCustomConfigNoUsableRemoteReportsErrorNode()
{
    // IPv6-literal remotes are skipped at resolve (v6 endpoints unsupported), leaving no usable
    // node: the descriptor must report an error node instead of indexing an empty remotes list.
    auto li = makeOvpnCustomConfigLocationInfo("dev tun\nproto udp\nauth-user-pass\nremote ::1 443\n");
    QVERIFY(!li.isNull());

    CustomConfigConnectionAttemptStrategy strategy(li);
    QSignalSpy resolvedSpy(&strategy, &IConnectionAttemptStrategy::hostnamesResolved);
    strategy.resolveHostnames();
    QCOMPARE(resolvedSpy.count(), 1);

    QCOMPARE(strategy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
}

void TestConnectionAttemptStrategy::testFactoryAutoAlwaysOnPlusKeepsWireguardWithCachedConfig()
{
    auto li = makeLocationInfo();

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, true),
                               makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2}), types::ProxySettings(), "", true, true, false));

    QVERIFY(strategy->isAutomaticMode());
    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    strategy->putFailedConnection();
    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    strategy->putFailedConnection();
    QCOMPARE(strategy->getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::IKEV2));
}

void TestConnectionAttemptStrategy::testFactoryManualWireguardSubstitutionEmptyPortsGuard()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2, types::Protocol::OPENVPN_UDP});
#ifdef Q_OS_LINUX
    // The lookup skips unsupported IKEv2 on Linux; exercise the empty-ports guard on the UDP item.
    portMap.items()[2].ports = {};
#else
    portMap.items()[1].ports = {};
    portMap.items()[2].ports = {1194};
#endif

    ConnectionAttemptStrategyFactory factory;
    QScopedPointer<IConnectionAttemptStrategy> strategy(
        factory.createStrategy(li, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                               portMap, types::ProxySettings(), "", true, false, false));

    // An item with no ports is found but unusable: no substitution happens, and the lookup
    // does not fall through past it.
    QVERIFY(!strategy->isAutomaticMode());
    const CurrentConnectionDescr descr = strategy->getCurrentConnectionSettings();
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(descr.port, 443u);
}

void TestConnectionAttemptStrategy::testAutoWalkSkipsEmptyPortsItem()
{
    auto li = makeLocationInfo();
    auto portMap = makePortMap({types::Protocol::WIREGUARD, types::Protocol::IKEV2});
    portMap.items()[1].ports = {};

    AutoConnectionAttemptStrategy strategy(li, portMap, false, false, false, "");
    QSignalSpy spy(&strategy, &IConnectionAttemptStrategy::protocolStatusChanged);

    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().protocol, types::Protocol(types::Protocol::WIREGUARD));
    strategy.putFailedConnection();
    QVERIFY(strategy.isFailed());

    QCOMPARE(spy.count(), 1);
    const auto status = spy.at(0).at(0).value<QVector<types::ProtocolStatus>>();
    QCOMPARE(status.size(), 1);
    QCOMPARE(status[0].protocol, types::Protocol(types::Protocol::WIREGUARD));
    QCOMPARE(status[0].status, types::ProtocolStatus::kFailed);
}

void TestConnectionAttemptStrategy::testManualStrategyResetClearsFailureCount()
{
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::IKEV2, 500, false),
                                             makePortMap({types::Protocol::IKEV2}), "");

    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());

    strategy.reset();

    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
}

void TestConnectionAttemptStrategy::testManualPreResolveProtocolMatchesSettings()
{
    auto li = makeLocationInfo();

    ManualConnectionAttemptStrategy strategy(li, types::ConnectionSettings(types::Protocol::OPENVPN_TCP, 443, false),
                                             makePortMap({types::Protocol::OPENVPN_TCP}), "");

    QCOMPARE(strategy.preResolveProtocol(), types::Protocol(types::Protocol::OPENVPN_TCP));
}

void TestConnectionAttemptStrategy::testDefaultPolicyVirtualsAutoAndManual()
{
    auto autoLi = makeLocationInfo();
    AutoConnectionAttemptStrategy autoStrategy(autoLi, makePortMap({types::Protocol::WIREGUARD}), false, false, false, "");
    auto manualLi = makeLocationInfo();
    ManualConnectionAttemptStrategy manualStrategy(manualLi, types::ConnectionSettings(types::Protocol::IKEV2, 500, false),
                                                   makePortMap({types::Protocol::IKEV2}), "");

    for (IConnectionAttemptStrategy *strategy : {static_cast<IConnectionAttemptStrategy *>(&autoStrategy),
                                                 static_cast<IConnectionAttemptStrategy *>(&manualStrategy)}) {
        QVERIFY(strategy->shouldWaitForNetwork());
        QVERIFY(!strategy->shouldRetryOnAttemptFailure());
        QVERIFY(strategy->usesConnectTimeout());
        QVERIFY(!strategy->surfacesTunnelTestFailure());
    }
    QVERIFY(!manualStrategy.hasProtocolChanged());
}

void TestConnectionAttemptStrategy::testResolveHostnamesEmitsForAutoAndManual()
{
    auto autoLi = makeLocationInfo();
    AutoConnectionAttemptStrategy autoStrategy(autoLi, makePortMap({types::Protocol::WIREGUARD}), false, false, false, "");
    auto manualLi = makeLocationInfo();
    ManualConnectionAttemptStrategy manualStrategy(manualLi, types::ConnectionSettings(types::Protocol::IKEV2, 500, false),
                                                   makePortMap({types::Protocol::IKEV2}), "");

    QSignalSpy autoSpy(&autoStrategy, &IConnectionAttemptStrategy::hostnamesResolved);
    QSignalSpy manualSpy(&manualStrategy, &IConnectionAttemptStrategy::hostnamesResolved);

    autoStrategy.resolveHostnames();
    manualStrategy.resolveHostnames();

    QCOMPARE(autoSpy.count(), 1);
    QCOMPARE(manualSpy.count(), 1);
}

void TestConnectionAttemptStrategy::testCachedConfigBudgetEdges()
{
    auto manualLi = makeLocationInfo();
    ManualConnectionAttemptStrategy manualStrategy(manualLi, types::ConnectionSettings(types::Protocol::WIREGUARD, 443, false),
                                                   makePortMap({types::Protocol::WIREGUARD}), "");
    manualStrategy.setCachedConfigAvailability(true);

    QVERIFY(!manualStrategy.isCachedConfigExhausted());
    QCOMPARE(manualStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::UseCachedOnly);
    // attempts == kMaxCachedConfigAttempts - 1: budget not exhausted yet.
    QCOMPARE(manualStrategy.cachedConfigAttempts(), IConnectionAttemptStrategy::kMaxCachedConfigAttempts - 1);
    QVERIFY(!manualStrategy.isCachedConfigExhausted());
    QCOMPARE(manualStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::UseCachedOnly);
    QCOMPARE(manualStrategy.cachedConfigAttempts(), IConnectionAttemptStrategy::kMaxCachedConfigAttempts);
    QVERIFY(manualStrategy.isCachedConfigExhausted());
    QCOMPARE(manualStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::Abort);

    manualStrategy.resetCachedConfigBudget();
    QVERIFY(!manualStrategy.isCachedConfigExhausted());

    auto autoLi = makeLocationInfo();
    AutoConnectionAttemptStrategy autoStrategy(autoLi, makePortMap({types::Protocol::WIREGUARD}), false, false, false, "");
    autoStrategy.setCachedConfigAvailability(true);

    QCOMPARE(autoStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::UseCachedOnly);
    QCOMPARE(autoStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::UseCachedOnly);
    QVERIFY(autoStrategy.isCachedConfigExhausted());
    QCOMPARE(autoStrategy.takeCachedConfigAdvice(), IConnectionAttemptStrategy::CachedConfigAdvice::Advance);
}

QTEST_MAIN(TestConnectionAttemptStrategy)
