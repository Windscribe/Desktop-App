#include <QSignalSpy>
#include <QtTest>

#include "openvpnconnection.test.h"

#include <memory>

#include "api_responses/servercredentials.h"
#include "engine/connectionmanager/connectors/openvpn/makeovpnfile.h"
#include "engine/connectionmanager/connectors/openvpn/openvpnconnection.h"
#include "extraconfig_mock.h"
#include "fakes.h"
#include "types/enums.h"
#include "utils/log/logger.h"

namespace {

api_responses::ServerCredentials makeCredentials(const QString &username, const QString &password)
{
    const QString json = QString("{\"data\":{\"username\":\"%1\",\"password\":\"%2\"}}")
                             .arg(QString(username.toUtf8().toBase64()), QString(password.toUtf8().toBase64()));
    return api_responses::ServerCredentials(json.toStdString());
}

OpenVpnSessionParams makeSessionParams()
{
    OpenVpnSessionParams params;
    params.ovpnConfig = "dev tun";
    params.serverCredentials = makeCredentials("session-user", "session-pass");
    return params;
}

CurrentConnectionDescr makeUdpDescr()
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_DEFAULT;
    d.protocol = types::Protocol::OPENVPN_UDP;
    d.ip = "10.1.2.3";
    d.hostname = "ovpn.example.com";
    d.port = 443;
    d.verifyX509name = "x509.example.com";
    return d;
}

} // namespace

void TestOpenVPNConnection::initTestCase()
{
    QCoreApplication::setOrganizationName("WindscribeTest");
    QCoreApplication::setApplicationName("openvpnconnection.test");
    qRegisterMetaType<ConnectError>("ConnectError");
}

void TestOpenVPNConnection::init()
{
    helper_ = new Helper(std::unique_ptr<IHelperBackend>(new FakeHelperBackend()),
                         log_utils::Logger::instance().getSpdLogger("basic"));
    ExtraConfigMock::reset();
}

void TestOpenVPNConnection::cleanup()
{
    delete helper_;
    helper_ = nullptr;
}

AttemptEnvironment TestOpenVPNConnection::makeEnv()
{
    AttemptEnvironment env;
    env.packetSize.isAutomatic = true;
    env.defaultAdapterInfo.setAdapterName("fake0");
    env.defaultAdapterInfo.addGatewayIp(types::IpAddress("192.168.1.1"));
    env.primaryDnsServer = "10.255.255.2";
    return env;
}

void TestOpenVPNConnection::testPrepareUdpUsesSessionCredentials()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);
    QSignalSpy failedSpy(&conn, &IConnection::prepareFailed);

    conn.prepare(makeUdpDescr(), makeEnv());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QVERIFY(conn.config_.startsWith("dev tun"));
    QVERIFY(conn.config_.contains("remote 10.1.2.3"));
    QVERIFY(conn.config_.contains("port 443"));
    QVERIFY(conn.config_.contains("proto udp"));
    QVERIFY(conn.config_.contains("verify-x509-name x509.example.com name"));
    QVERIFY(conn.config_.contains("dhcp-option DNS 10.255.255.2"));
    // Automatic packet size: no explicit MSS clamp.
    QVERIFY(!conn.config_.contains("mssfix"));
    QCOMPARE(conn.username_, QString("session-user"));
    QCOMPARE(conn.password_, QString("session-pass"));
    QVERIFY(!conn.isCustomConfig_);
}

void TestOpenVPNConnection::testPrepareRefinesProtocolToDialedVariant()
{
    // A connector built with the pre-resolve family representative (emergency, custom) takes the
    // attempt's dialed variant at prepare().
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);

    CurrentConnectionDescr descr = makeUdpDescr();
    descr.protocol = types::Protocol::OPENVPN_TCP;
    conn.prepare(descr, makeEnv());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.protocol_, types::Protocol::OPENVPN_TCP);
    QVERIFY(conn.config_.contains("proto tcp-client"));
}

void TestOpenVPNConnection::testPrepareStaticIpUsesDescrCredentials()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    CurrentConnectionDescr descr = makeUdpDescr();
    descr.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
    descr.staticIps.credentials = api_responses::ServerCredentials("static-user", "static-pass");

    conn.prepare(descr, makeEnv());

    QCOMPARE(conn.username_, QString("static-user"));
    QCOMPARE(conn.password_, QString("static-pass"));
}

void TestOpenVPNConnection::testPrepareManualPacketSizeAddsMss()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    AttemptEnvironment env = makeEnv();
    env.packetSize.isAutomatic = false;
    env.packetSize.mtu = 1500;

    conn.prepare(makeUdpDescr(), env);

    // Default OpenVPN MTU offset is 40.
    QVERIFY(conn.config_.contains("mssfix 1460"));
}

void TestOpenVPNConnection::testPrepareManualPacketSizeHonorsExtraConfigOffset()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    ExtraConfigMock::hasMtuOffsetOpenVpn = true;
    ExtraConfigMock::mtuOffsetOpenVpn = 100;
    AttemptEnvironment env = makeEnv();
    env.packetSize.isAutomatic = false;
    env.packetSize.mtu = 1500;

    conn.prepare(makeUdpDescr(), env);

    QVERIFY(conn.config_.contains("mssfix 1400"));
}

void TestOpenVPNConnection::testPrepareMssTooLowOmitsMssfix()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    AttemptEnvironment env = makeEnv();
    env.packetSize.isAutomatic = false;
    env.packetSize.mtu = 30;

    conn.prepare(makeUdpDescr(), env);

    QVERIFY(!conn.config_.contains("mssfix"));
}

void TestOpenVPNConnection::testPrepareAntiCensorshipAddsStuffing()
{
    // The emergency session carries the user's anti-censorship setting without an amnezia preset.
    OpenVpnSessionParams params = makeSessionParams();
    params.isAntiCensorship = true;
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, params);

    conn.prepare(makeUdpDescr(), makeEnv());

    QVERIFY(conn.config_.contains("udp-stuffing"));
    QVERIFY(conn.config_.contains("tcp-split-reset"));
}

void TestOpenVPNConnection::testPrepareCustomConfigInjectsDnsAndClearsCredentials()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);
    CurrentConnectionDescr descr;
    descr.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    descr.protocol = types::Protocol::OPENVPN_UDP;
    descr.ip = "5.6.7.8";
    // The descriptor carries the finished .ovpn text, assembled upstream with the resolved node ip.
    descr.openVpn.customConfig = "dev tun\r\nauth-user-pass\r\nremote 5.6.7.8 443\r\n";

    conn.prepare(descr, makeEnv());

    QCOMPARE(preparedSpy.count(), 1);
    QVERIFY(conn.config_.contains("remote 5.6.7.8 443"));
    // The override is a pair: the pull-filter drops the server-pushed DNS, the dhcp-option sets ours.
    QVERIFY(conn.config_.contains("pull-filter ignore \"dhcp-option DNS\""));
    QVERIFY(conn.config_.contains("dhcp-option DNS 10.255.255.2"));
    QVERIFY(conn.isCustomConfig_);
    // Custom-config credentials arrive via the user-input path, never from the session.
    QVERIFY(conn.username_.isEmpty());
    QVERIFY(conn.password_.isEmpty());
}

void TestOpenVPNConnection::testPrepareInvalidNodeIpFails()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);
    QSignalSpy failedSpy(&conn, &IConnection::prepareFailed);
    CurrentConnectionDescr descr = makeUdpDescr();
    descr.ip = "not-an-ip";

    conn.prepare(descr, makeEnv());

    QCOMPARE(preparedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(failedSpy.at(0).at(0)), ConnectError::kLocalProcessLaunchFailure);
}

void TestOpenVPNConnection::testParsePushReplyRedirectGateway()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    const QString reply = ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,redirect-gateway def1,"
                          "route-gateway 10.255.255.1,topology subnet,ping 10,ping-restart 60,"
                          "ifconfig 10.255.255.6 255.255.255.0,dhcp-option DNS 10.255.255.3,dhcp-option DNS 10.255.255.4,peer-id 0'";
    AdapterGatewayInfo info;
    bool redirect = false;

    QVERIFY(conn.parsePushReply(reply, info, redirect));

    QVERIFY(redirect);
    QCOMPARE(QString::fromStdString(info.adapterIpV4().toString()), QString("10.255.255.6"));
    QCOMPARE(QString::fromStdString(info.gatewayV4().toString()), QString("10.255.255.1"));
    QCOMPARE(info.dnsServersAsStringList(), QStringList({"10.255.255.3", "10.255.255.4"}));
}

void TestOpenVPNConnection::testParsePushReplyNoRedirectGateway()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    const QString reply = ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,route-gateway 10.255.255.1,"
                          "ifconfig 10.255.255.6 255.255.255.0,dhcp-option DNS 10.255.255.3'";
    AdapterGatewayInfo info;
    bool redirect = true;

    QVERIFY(conn.parsePushReply(reply, info, redirect));

    QVERIFY(!redirect);
    QCOMPARE(QString::fromStdString(info.adapterIpV4().toString()), QString("10.255.255.6"));
}

void TestOpenVPNConnection::testParsePushReplyMalformed()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    const QStringList malformed = {
        // No quoted payload.
        ">LOG:1602589553,,PUSH: Received control message: PUSH_REPLY,route-gateway 10.255.255.1",
        // route-gateway missing its address.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,route-gateway'",
        // route-gateway with a non-IP address.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,route-gateway not-an-ip'",
        // ifconfig missing the netmask.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,ifconfig 10.255.255.6'",
        // ifconfig with a non-IP address.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,ifconfig not-an-ip 255.255.255.0'",
        // dhcp-option DNS with a non-IP address.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,dhcp-option DNS not-an-ip'",
        // dhcp-option truncated.
        ">LOG:1602589553,,PUSH: Received control message: 'PUSH_REPLY,dhcp-option DNS'",
    };

    for (const QString &reply : malformed) {
        AdapterGatewayInfo info;
        bool redirect = true;
        QVERIFY2(!conn.parsePushReply(reply, info, redirect), qPrintable(reply));
    }
}

void TestOpenVPNConnection::testParseDeviceOpenedReply()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    struct Row {
        QString reply;
        QString device;
    };
    const QVector<Row> rows = {
        // mac utun form.
        {">LOG:1602589552,,Opened utun device utun3", "utun3"},
        // mac capital-D variant.
        {">LOG:1602589552,,Opened utun Device utun5", "utun5"},
        // linux tun/tap form.
        {">LOG:1602589552,I,TUN/TAP device tun0 opened", "tun0"},
        // linux DCO form.
        {">LOG:1602589552,I,DCO device tun0 opened", "tun0"},
    };

    for (const Row &row : rows) {
        QString device;
        QVERIFY2(conn.parseDeviceOpenedReply(row.reply, device), qPrintable(row.reply));
        QCOMPARE(device, row.device);
    }
}

void TestOpenVPNConnection::testParseDeviceOpenedReplyMalformed()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    const QStringList malformed = {
        // Fewer than three comma fields.
        ">LOG:1602589552,Opened utun device utun3",
        // No 'device' keyword.
        ">LOG:1602589552,,Opened utun utun3",
        // 'device' is the last word.
        ">LOG:1602589552,,Opened utun device",
    };

    for (const QString &reply : malformed) {
        QString device;
        QVERIFY2(!conn.parseDeviceOpenedReply(reply, device), qPrintable(reply));
    }
}

void TestOpenVPNConnection::testParseConnectedSuccessReply()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QString remoteIp;

    QVERIFY(conn.parseConnectedSuccessReply(">STATE:1602589554,CONNECTED,SUCCESS,10.255.255.6,185.204.1.174,1194,,", remoteIp));

    QCOMPARE(remoteIp, QString("185.204.1.174"));
}

void TestOpenVPNConnection::testParseConnectedSuccessReplyMalformed()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    const QStringList malformed = {
        // Fewer than five comma fields.
        ">STATE:1602589554,CONNECTED,SUCCESS,10.255.255.6",
        // Remote ip field present but empty.
        ">STATE:1602589554,CONNECTED,SUCCESS,10.255.255.6,,1194",
    };

    for (const QString &reply : malformed) {
        QString remoteIp;
        QVERIFY2(!conn.parseConnectedSuccessReply(reply, remoteIp), qPrintable(reply));
    }
}

void TestOpenVPNConnection::testSanitizeString()
{
    // Credentials are embedded in a quoted management command: CR/LF would inject a second command,
    // so they are stripped; backslash, quote and tab are escaped per management-notes.txt.
    struct Row {
        QString in;
        QString out;
    };
    const QVector<Row> rows = {
        {"plain-pass", "plain-pass"},
        {"pa\"ss", "pa\\\"ss"},
        {"a\\b", "a\\\\b"},
        {"a\tb", "a\\tb"},
        {"a\rb\nc", "abc"},
        // Backslash is escaped before the quote so the pair stays two escapes, not a re-escape.
        {"\\\"", "\\\\\\\""},
        {"user\nsignal SIGTERM\n", "usersignal SIGTERM"},
    };

    for (const Row &row : rows) {
        QCOMPARE(OpenVPNConnection::sanitizeString(row.in), row.out);
    }
}

void TestOpenVPNConnection::testMakeOvpnFileStunnelUsesLocalWrapper()
{
    for (types::Protocol protocol : {types::Protocol::STUNNEL, types::Protocol::WSTUNNEL}) {
        MakeOVPNFile file;
        QVERIFY(file.generate("dev tun", "10.1.2.3", protocol, 443, 15501, 0, "192.168.1.1", "", "", false, ""));

        const QString config = file.config();
        QVERIFY(config.contains("remote 127.0.0.1"));
        QVERIFY(config.contains("port 15501"));
        QVERIFY(config.contains("proto tcp-client"));
        QVERIFY(!config.contains("remote 10.1.2.3"));
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        // The wrapper dials the node itself; the direct host route keeps that traffic off the tunnel.
        QVERIFY(config.contains("route 10.1.2.3 255.255.255.255 192.168.1.1"));
#endif
    }
}

void TestOpenVPNConnection::testMakeOvpnFileRemoteIpSuppression()
{
    // When the advanced params carry their own remote, the generated config must not add one; the
    // override line itself comes from the extra-config text (empty in the mock), so none appears.
    ExtraConfigMock::remoteIp = "104.20.20.20";
    MakeOVPNFile file;

    QVERIFY(file.generate("dev tun", "10.1.2.3", types::Protocol::OPENVPN_UDP, 443, 0, 0, "", "", "", false, ""));

    QVERIFY(!file.config().contains("remote "));
    QVERIFY(file.config().contains("port 443"));
    QVERIFY(file.config().contains("proto udp"));

    // With the override remote carried in the extra-config text, that remote is the only one emitted.
    ExtraConfigMock::extraConfigForOpenVpn = "remote 104.20.20.20\r\n";
    MakeOVPNFile overrideFile;
    QVERIFY(overrideFile.generate("dev tun", "10.1.2.3", types::Protocol::OPENVPN_UDP, 443, 0, 0, "", "", "", false, ""));
    QVERIFY(!overrideFile.config().contains("remote 10.1.2.3"));
    QVERIFY(overrideFile.config().contains("remote 104.20.20.20"));
}

void TestOpenVPNConnection::testMakeOvpnFileExtraConfigAppend()
{
    ExtraConfigMock::extraConfigForOpenVpn = "verb 4\r\n";
    MakeOVPNFile file;

    QVERIFY(file.generate("dev tun", "10.1.2.3", types::Protocol::OPENVPN_UDP, 443, 0, 0, "", "", "", false, ""));

    QVERIFY(file.config().contains("verb 4"));
}

void TestOpenVPNConnection::testMakeOvpnFileTcpBranch()
{
    MakeOVPNFile file;

    QVERIFY(file.generate("dev tun", "10.1.2.3", types::Protocol::OPENVPN_TCP, 443, 0, 1460, "", "", "", false, ""));

    const QString config = file.config();
    QVERIFY(config.contains("remote 10.1.2.3"));
    QVERIFY(config.contains("port 443"));
    QVERIFY(config.contains("proto tcp-client"));
    // TCP handles segmentation itself; the MSS clamp is a UDP-only directive.
    QVERIFY(!config.contains("mssfix"));
}

void TestOpenVPNConnection::testMakeOvpnFileStunnelNoGatewayNoRoute()
{
    MakeOVPNFile file;

    QVERIFY(file.generate("dev tun", "10.1.2.3", types::Protocol::STUNNEL, 443, 15501, 0, "", "", "", false, ""));

    QVERIFY(file.config().contains("remote 127.0.0.1"));
    QVERIFY(!file.config().contains("route "));
}

void TestOpenVPNConnection::testPrepareCustomConfigEmptyDnsNoInjection()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);
    CurrentConnectionDescr descr;
    descr.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    descr.protocol = types::Protocol::OPENVPN_UDP;
    descr.ip = "5.6.7.8";
    descr.openVpn.customConfig = "dev tun\r\nauth-user-pass\r\nremote 5.6.7.8 443\r\n";
    AttemptEnvironment env = makeEnv();
    env.primaryDnsServer = "";

    conn.prepare(descr, env);

    QCOMPARE(preparedSpy.count(), 1);
    // No connected-DNS override: the custom config is used verbatim, its own DNS untouched.
    QCOMPARE(conn.config_, descr.openVpn.customConfig);
    QVERIFY(!conn.config_.contains("pull-filter"));
    QVERIFY(!conn.config_.contains("dhcp-option DNS"));
}

void TestOpenVPNConnection::testOpenVpnCapabilities()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QCOMPARE(conn.capabilities().connectTimeoutMs, 30 * 1000);
    QVERIFY(!conn.capabilities().supportsCachedConfig);
    QVERIFY(conn.capabilities().needsSystemDnsRestore);
}

QTEST_MAIN(TestOpenVPNConnection)
