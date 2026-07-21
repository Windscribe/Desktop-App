#include <QSignalSpy>
#include <QtTest>

#include "ikev2connection.test.h"

#include "engine/connectionmanager/connectors/ikev2/ikev2connectionbase.h"
#include "fakes.h"

namespace {

// The base is abstract (the dial surface stays platform-specific); tests only exercise prepare.
class StubIkev2Connection : public Ikev2ConnectionBase
{
public:
    explicit StubIkev2Connection(const Ikev2SessionParams &sessionParams)
        : Ikev2ConnectionBase(nullptr, types::Protocol::IKEV2, sessionParams) {}

    void startConnect() override {}
    void startDisconnect() override { emit disconnected(); }
    bool isDisconnected() const override { return true; }
};

Ikev2SessionParams makeSessionParams()
{
    Ikev2SessionParams params;
    params.serverCredentials = api_responses::ServerCredentials("session-user", "session-pass");
    return params;
}

CurrentConnectionDescr makeDescr()
{
    CurrentConnectionDescr d;
    d.connectionNodeType = CONNECTION_NODE_DEFAULT;
    d.protocol = types::Protocol::IKEV2;
    d.ip = "10.0.0.3";
    d.hostname = "ikev2-host.example.com";
    return d;
}

} // namespace

void TestIkev2Connection::testCapabilities()
{
    StubIkev2Connection conn{Ikev2SessionParams()};
    QCOMPARE(conn.capabilities().connectTimeoutMs, 30 * 1000);
    QVERIFY(!conn.capabilities().dualStackEgress);
    QVERIFY(!conn.capabilities().supportsCachedConfig);
    QVERIFY(!conn.capabilities().needsSystemDnsRestore);
}

void TestIkev2Connection::testPrepareKeepsEndpointWithoutExtraConfig()
{
    StubIkev2Connection conn{makeSessionParams()};
    QSignalSpy preparedSpy(&conn, &Ikev2ConnectionBase::prepared);

    FakeExtraConfig extraConfig;
    AttemptEnvironment env;
    env.extraConfig = &extraConfig;
    conn.prepare(makeDescr(), env);

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.effectiveHostname(), QString("ikev2-host.example.com"));
    QCOMPARE(conn.effectiveIp(), QString("10.0.0.3"));
}

void TestIkev2Connection::testPrepareExtraConfigDomainOverridesEndpoint()
{
    StubIkev2Connection conn{makeSessionParams()};
    QSignalSpy preparedSpy(&conn, &Ikev2ConnectionBase::prepared);

    FakeExtraConfig extraConfig;
    extraConfig.setRemoteIp("remote-override.example.com");
    extraConfig.setDetectedIp("5.6.7.8");
    AttemptEnvironment env;
    env.extraConfig = &extraConfig;
    conn.prepare(makeDescr(), env);

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.effectiveHostname(), QString("remote-override.example.com"));
    QCOMPARE(conn.effectiveIp(), QString("5.6.7.8"));
}

void TestIkev2Connection::testPrepareExtraConfigNonDomainIgnored()
{
    StubIkev2Connection conn{makeSessionParams()};
    QSignalSpy preparedSpy(&conn, &Ikev2ConnectionBase::prepared);

    FakeExtraConfig extraConfig;
    extraConfig.setRemoteIp("1.2.3.4");
    extraConfig.setDetectedIp("5.6.7.8");
    AttemptEnvironment env;
    env.extraConfig = &extraConfig;
    conn.prepare(makeDescr(), env);

    QCOMPARE(preparedSpy.count(), 1);
    QCOMPARE(conn.effectiveHostname(), QString("ikev2-host.example.com"));
    QCOMPARE(conn.effectiveIp(), QString("10.0.0.3"));
}

void TestIkev2Connection::testCredentialPickSessionForDefaultNode()
{
    StubIkev2Connection conn{makeSessionParams()};

    FakeExtraConfig extraConfig;
    AttemptEnvironment env;
    env.extraConfig = &extraConfig;
    CurrentConnectionDescr d = makeDescr();
    d.username = "static-user";
    d.password = "static-pass";
    conn.prepare(d, env);

    QCOMPARE(conn.username(), QString("session-user"));
    QCOMPARE(conn.password(), QString("session-pass"));
}

void TestIkev2Connection::testCredentialPickDescrForStaticIpNode()
{
    StubIkev2Connection conn{makeSessionParams()};

    FakeExtraConfig extraConfig;
    AttemptEnvironment env;
    env.extraConfig = &extraConfig;
    CurrentConnectionDescr d = makeDescr();
    d.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
    d.username = "static-user";
    d.password = "static-pass";
    conn.prepare(d, env);

    QCOMPARE(conn.username(), QString("static-user"));
    QCOMPARE(conn.password(), QString("static-pass"));
}

QTEST_MAIN(TestIkev2Connection)
