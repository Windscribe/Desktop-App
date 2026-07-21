#include <QSignalSpy>
#include <QtTest>

#include "openvpnconnection.test.h"

#include <memory>

#include "api_responses/servercredentials.h"
#include "engine/connectionmanager/connectors/openvpn/openvpnconnection.h"
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
    qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
}

void TestOpenVPNConnection::init()
{
    helper_ = new Helper(std::unique_ptr<IHelperBackend>(new FakeHelperBackend()),
                         log_utils::Logger::instance().getSpdLogger("basic"));
    extraConfig_ = new FakeExtraConfig();
}

void TestOpenVPNConnection::cleanup()
{
    delete extraConfig_;
    delete helper_;
    extraConfig_ = nullptr;
    helper_ = nullptr;
}

AttemptEnvironment TestOpenVPNConnection::makeEnv()
{
    AttemptEnvironment env;
    env.packetSize.isAutomatic = true;
    env.defaultAdapterInfo.setAdapterName("fake0");
    env.defaultAdapterInfo.addGatewayIp(types::IpAddress("192.168.1.1"));
    env.primaryDnsServer = "10.255.255.2";
    env.extraConfig = extraConfig_;
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
    descr.username = "static-user";
    descr.password = "static-pass";

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
    extraConfig_->setMtuOffsetOpenVpn(true, 100);
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

void TestOpenVPNConnection::testPrepareCustomConfigRewritesRemoteAndClearsCredentials()
{
    OpenVPNConnection conn(nullptr, helper_, types::Protocol::OPENVPN_UDP, makeSessionParams());
    QSignalSpy preparedSpy(&conn, &IConnection::prepared);
    CurrentConnectionDescr descr;
    descr.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    descr.protocol = types::Protocol::OPENVPN_UDP;
    descr.ip = "5.6.7.8";
    descr.ovpnData = "dev tun\r\nauth-user-pass\r\n";
    descr.remoteCmdLine = "remote 1.2.3.4 443";
    descr.customConfigFilename = "/configs/my.ovpn";

    conn.prepare(descr, makeEnv());

    QCOMPARE(preparedSpy.count(), 1);
    // The remote line's host is rewritten to the resolved node ip.
    QVERIFY(conn.config_.contains("remote 5.6.7.8 443"));
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
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(failedSpy.at(0).at(0)), CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
}

QTEST_MAIN(TestOpenVPNConnection)
