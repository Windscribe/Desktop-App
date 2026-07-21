#include <QSignalSpy>
#include <QtTest>

#include "emergencyconnsettingspolicy.test.h"

#include "engine/connectionmanager/connsettingspolicy/emergencyconnsettingspolicy.h"

void TestEmergencyConnSettingsPolicy::testDescrBeforeFetchIsError()
{
    EmergencyConnSettingsPolicy policy;
    policy.start();

    const CurrentConnectionDescr descr = policy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_ERROR);
    // The pre-resolve lockdown check in doConnect() reads the protocol; it must never look like IKEv2.
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QVERIFY(!policy.isFailed());
}

void TestEmergencyConnSettingsPolicy::testEndpointMappingAndIteration()
{
    EmergencyConnSettingsPolicy policy;
    policy.start();
    QSignalSpy resolvedSpy(&policy, &BaseConnSettingsPolicy::hostnamesResolved);
    policy.onEndpointsReceived({{"10.0.0.1", 443, false}, {"10.0.0.2", 1194, true}});
    QCOMPARE(resolvedSpy.count(), 1);

    CurrentConnectionDescr descr = policy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_DEFAULT);
    QCOMPARE(descr.ip, QString("10.0.0.1"));
    QCOMPARE(descr.port, 443u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));

    policy.putFailedConnection();
    QVERIFY(!policy.isFailed());
    descr = policy.getCurrentConnectionSettings();
    QCOMPARE(descr.ip, QString("10.0.0.2"));
    QCOMPARE(descr.port, 1194u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_TCP));

    policy.putFailedConnection();
    QVERIFY(policy.isFailed());
    QCOMPARE(policy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
}

void TestEmergencyConnSettingsPolicy::testEmptyEndpointListFails()
{
    EmergencyConnSettingsPolicy policy;
    policy.start();
    policy.onEndpointsReceived({});

    QVERIFY(policy.isFailed());
    QCOMPARE(policy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
}

void TestEmergencyConnSettingsPolicy::testResolveAfterFetchEmitsFromCache()
{
    EmergencyConnSettingsPolicy policy;
    policy.start();
    policy.onEndpointsReceived({{"10.0.0.1", 443, false}});

    // Subsequent attempts re-resolve; the cached list answers without another fetch.
    QSignalSpy resolvedSpy(&policy, &BaseConnSettingsPolicy::hostnamesResolved);
    policy.resolveHostnames();
    QCOMPARE(resolvedSpy.count(), 1);
    QCOMPARE(policy.getCurrentConnectionSettings().ip, QString("10.0.0.1"));
}

void TestEmergencyConnSettingsPolicy::testPreFetchFailureConsumesNoEndpoint()
{
    // A connecting timeout can fail the attempt while the endpoint fetch is still in flight; that
    // failure must not skip an endpoint that was never attempted.
    EmergencyConnSettingsPolicy policy;
    policy.start();
    policy.putFailedConnection();
    policy.onEndpointsReceived({{"10.0.0.1", 443, false}});

    QCOMPARE(policy.getCurrentConnectionSettings().ip, QString("10.0.0.1"));
    QVERIFY(!policy.isFailed());
}

void TestEmergencyConnSettingsPolicy::testResetClearsFetchedState()
{
    EmergencyConnSettingsPolicy policy;
    policy.start();
    policy.onEndpointsReceived({{"10.0.0.1", 443, false}});
    policy.putFailedConnection();

    policy.reset();

    QVERIFY(!policy.isFailed());
    QCOMPARE(policy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
    // A fresh fetch restarts iteration at the head of the new list.
    policy.onEndpointsReceived({{"10.0.0.9", 80, true}});
    QCOMPARE(policy.getCurrentConnectionSettings().ip, QString("10.0.0.9"));
}

void TestEmergencyConnSettingsPolicy::testStaticAnswers()
{
    EmergencyConnSettingsPolicy policy;
    QCOMPARE(policy.preResolveProtocol(), types::Protocol(types::Protocol::OPENVPN_UDP));
    QVERIFY(!policy.isAutomaticMode());
    QVERIFY(!policy.isCustomConfig());
    QVERIFY(!policy.hasProtocolChanged());
    QVERIFY(!policy.shouldWaitForNetwork());
}

QTEST_MAIN(TestEmergencyConnSettingsPolicy)
