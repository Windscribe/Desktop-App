#include <QSignalSpy>
#include <QtTest>

#include "wireguardconnection.test.h"

#include "api_responses/amneziawgunblockparams.h"
#include "engine/connectionmanager/connectors/wireguard/wireguardconnectionbase.h"
#include "engine/wireguardconfig/getwireguardconfig.h"
#include "types/enums.h"

namespace {

// The base is abstract (the dial surface stays platform-specific); tests only exercise prepare.
class StubWireGuardConnection : public WireGuardConnectionBase
{
public:
    explicit StubWireGuardConnection(const WireGuardSessionParams &sessionParams)
        : WireGuardConnectionBase(nullptr, types::Protocol::WIREGUARD, sessionParams) {}

    void startConnect() override {}
    void startDisconnect() override { emit disconnected(); }
    bool isDisconnected() const override { return true; }
};

WireGuardConfig makeDualStackConfig()
{
    WireGuardConfig config;
    config.setClientIpAddress("10.245.6.78/32, fd00:abcd::2/128");
    config.setClientDnsAddress("10.255.255.2, fd00:abcd::1");
    config.setPeerAllowedIPs("0.0.0.0/0, ::/0");
    return config;
}

CurrentConnectionDescr makeDescr(bool isIpv6Support = true)
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_DEFAULT;
    d.protocol = types::Protocol::WIREGUARD;
    d.ip = "10.0.0.3";
    d.hostname = "wireguard.example.com";
    d.port = 51820;
    d.wireGuard.peerPublicKey = "peer-public-key";
    d.isIpv6Support = isIpv6Support;
    return d;
}

} // namespace

void TestWireGuardConnection::initTestCase()
{
    qRegisterMetaType<ConnectError>("ConnectError");
    qRegisterMetaType<UserInputType>("UserInputType");
}

void TestWireGuardConnection::testCapabilities()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    QCOMPARE(conn.capabilities().connectTimeoutMs, 20 * 1000);
    QVERIFY(conn.capabilities().supportsCachedConfig);
    QVERIFY(conn.capabilities().needsSystemDnsRestore);
}

void TestWireGuardConnection::testCustomConfigPrepareKeepsDualStack()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wireGuard.customConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    conn.prepare(d, AttemptEnvironment());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32, fd00:abcd::2/128"));
    QCOMPARE(conn.tunnelDefaultDns(), QString("10.255.255.2, fd00:abcd::1"));
}

void TestWireGuardConnection::testCustomConfigPrepareStripsIpv6WhenIpv4Only()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wireGuard.customConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    AttemptEnvironment env;
    env.ipStackEgress = IpStack::kIPv4Only;
    conn.prepare(d, env);

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32"));
    QCOMPARE(conn.tunnelDefaultDns(), QString("10.255.255.2"));
}

void TestWireGuardConnection::testDialConfigAppliesDnsOverrideKeepsConfigPristine()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wireGuard.customConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    AttemptEnvironment env;
    env.primaryDnsServer = "192.168.100.53";
    conn.prepare(d, env);

    // Firewall-whitelist contract: the dial carries the override, but the stored config keeps its
    // own DNS so tunnelDefaultDns() still reports it.
    QCOMPARE(conn.dialConfig().clientDnsAddress(), QString("192.168.100.53"));
    QCOMPARE(conn.config_.clientDnsAddress(), QString("10.255.255.2, fd00:abcd::1"));
    QCOMPARE(conn.tunnelDefaultDns(), QString("10.255.255.2, fd00:abcd::1"));
}

void TestWireGuardConnection::testTunnelDefaultDnsEmptyNotNullAfterIpv4Strip()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};

    // v6-only DNS + IPv4-only strip leaves the stored DNS null; the readback must be empty, not
    // null, or the manager would skip whitelisting the connected-DNS override.
    WireGuardConfig config = makeDualStackConfig();
    config.setClientDnsAddress("fd00:abcd::1");
    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wireGuard.customConfig = QSharedPointer<WireGuardConfig>::create(config);
    AttemptEnvironment env;
    env.ipStackEgress = IpStack::kIPv4Only;
    conn.prepare(d, env);

    const QString dns = conn.tunnelDefaultDns();
    QVERIFY(dns.isEmpty());
    QVERIFY(!dns.isNull());
}

void TestWireGuardConnection::testAnswerSuccessDualStack()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 1);
    QVERIFY(conn.isDualStackEgress());
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32, fd00:abcd::2/128"));
    QCOMPARE(conn.config_.peerPublicKey(), QString("peer-public-key"));
    QCOMPARE(conn.config_.peerEndpoint(), QString("10.0.0.3:51820"));
}

void TestWireGuardConnection::testAnswerSuccessStripsWithoutNodeIpv6()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr(/*isIpv6Support=*/false);
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 1);
    QVERIFY(!conn.isDualStackEgress());
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32"));
}

void TestWireGuardConnection::testAnswerSuccessStripsWhenIpv4Only()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    conn.env_.ipStackEgress = IpStack::kIPv4Only;
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32"));
}

void TestWireGuardConnection::testAnswerSuccessInvalidNodeIp()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    CurrentConnectionDescr d = makeDescr();
    d.ip = "not-an-ip";
    conn.descr_ = d;
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);
    QSignalSpy failedSpy(&conn, &WireGuardConnectionBase::prepareFailed);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(failedSpy.at(0).at(0)), ConnectError::kTunnelEstablishmentFailure);
}

void TestWireGuardConnection::testAnswerAppliesAmneziaPresetParam()
{
    WireGuardSessionParams params;
    params.amneziawgPreset = "Preset A";
    params.amneziawgParams = api_responses::AmneziawgUnblockParams(
        R"({"data":{"params":[{"id":"a","title":"Preset A","Jc":4,"Jmin":40,"Jmax":70}]}})");
    StubWireGuardConnection conn{params};
    conn.descr_ = makeDescr();
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 1);
    QVERIFY(conn.config_.haveAmneziawgParam());
    QCOMPARE(conn.config_.amneziawgParamTitle(), QString("Preset A"));
}

void TestWireGuardConnection::testAnswerEmptyPresetClearsAmneziaParam()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    // A cached config can arrive with a leftover param; an empty preset must clear it.
    WireGuardConfig fetched = makeDualStackConfig();
    api_responses::AmneziawgUnblockParam stale;
    stale.setDefault();
    fetched.setAmneziawgParam(stale);
    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, fetched);

    QCOMPARE(preparedSpy.count(), 1);
    QVERIFY(!conn.config_.haveAmneziawgParam());
}

void TestWireGuardConnection::testAnswerKeyLimit()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy inputSpy(&conn, &WireGuardConnectionBase::userInputRequired);
    QSignalSpy failedSpy(&conn, &WireGuardConnectionBase::prepareFailed);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kKeyLimit, WireGuardConfig());

    QCOMPARE(inputSpy.count(), 1);
    QVERIFY(qvariant_cast<UserInputType>(inputSpy.at(0).at(0)) == UserInputType::KeyLimitConsent);
    QCOMPARE(failedSpy.count(), 0);
}

void TestWireGuardConnection::testAnswerFailoverFailed()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy failedSpy(&conn, &WireGuardConnectionBase::prepareFailed);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kFailoverFailed, WireGuardConfig());

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(failedSpy.at(0).at(0)), ConnectError::kConfigFetchFailure);
}

void TestWireGuardConnection::testAnswerFailed()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy failedSpy(&conn, &WireGuardConnectionBase::prepareFailed);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, WireGuardConfig());

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(qvariant_cast<ConnectError>(failedSpy.at(0).at(0)), ConnectError::kLocalConfigGenerationFailure);
}

void TestWireGuardConnection::testDialConfigEmptyPrimaryDnsKeepsConfigDns()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wireGuard.customConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    conn.prepare(d, AttemptEnvironment());

    // No connected-DNS override: the dial keeps the config's own DNS servers.
    QCOMPARE(conn.dialConfig().clientDnsAddress(), QString("10.255.255.2, fd00:abcd::1"));
}

QTEST_MAIN(TestWireGuardConnection)
