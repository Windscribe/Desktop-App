#include <QSignalSpy>
#include <QtTest>

#include "wireguardconnection.test.h"

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
    d.wgPeerPublicKey = "peer-public-key";
    d.isIpv6Support = isIpv6Support;
    return d;
}

} // namespace

void TestWireGuardConnection::initTestCase()
{
    qRegisterMetaType<CONNECT_ERROR>("CONNECT_ERROR");
    qRegisterMetaType<UserInputType>("UserInputType");
}

void TestWireGuardConnection::testCapabilities()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    QCOMPARE(conn.capabilities().connectTimeoutMs, 20 * 1000);
    QVERIFY(conn.capabilities().dualStackEgress);
    QVERIFY(conn.capabilities().supportsCachedConfig);
    QVERIFY(conn.capabilities().needsSystemDnsRestore);
}

void TestWireGuardConnection::testCustomConfigPrepareKeepsDualStack()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wgCustomConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    conn.prepare(d, AttemptEnvironment());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32, fd00:abcd::2/128"));
    QCOMPARE(conn.tunnelDefaultDns(), QString("10.255.255.2, fd00:abcd::1"));
}

void TestWireGuardConnection::testCustomConfigPrepareStripsIpv6WhenIpv4Only()
{
    WireGuardSessionParams params;
    params.ipStackEgress = IpStack::kIPv4Only;
    StubWireGuardConnection conn{params};
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_CUSTOM_CONFIG;
    d.wgCustomConfig = QSharedPointer<WireGuardConfig>::create(makeDualStackConfig());
    conn.prepare(d, AttemptEnvironment());

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32"));
    QCOMPARE(conn.tunnelDefaultDns(), QString("10.255.255.2"));
}

void TestWireGuardConnection::testAnswerSuccessDualStack()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy preparedSpy(&conn, &WireGuardConnectionBase::prepared);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, makeDualStackConfig());

    QCOMPARE(preparedSpy.count(), 1);
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
    QCOMPARE(conn.config_.clientIpAddress(), QString("10.245.6.78/32"));
}

void TestWireGuardConnection::testAnswerSuccessStripsWhenIpv4Only()
{
    WireGuardSessionParams params;
    params.ipStackEgress = IpStack::kIPv4Only;
    StubWireGuardConnection conn{params};
    conn.descr_ = makeDescr();
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
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(failedSpy.at(0).at(0)), CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
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
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(failedSpy.at(0).at(0)), CONNECT_ERROR::WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
}

void TestWireGuardConnection::testAnswerFailed()
{
    StubWireGuardConnection conn{WireGuardSessionParams()};
    conn.descr_ = makeDescr();
    QSignalSpy failedSpy(&conn, &WireGuardConnectionBase::prepareFailed);

    conn.onGetWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, WireGuardConfig());

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(qvariant_cast<CONNECT_ERROR>(failedSpy.at(0).at(0)), CONNECT_ERROR::CONFIG_FETCH_FAILED);
}

QTEST_MAIN(TestWireGuardConnection)
