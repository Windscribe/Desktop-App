#include <QSignalSpy>
#include <QtTest>

#include "emergencyconnectionattemptstrategy.test.h"

#include "engine/connectionmanager/attemptstrategy/emergencyconnectionattemptstrategy.h"

void TestEmergencyConnectionAttemptStrategy::testDescrBeforeFetchIsError()
{
    EmergencyConnectionAttemptStrategy strategy;

    const CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_ERROR);
    // The pre-resolve lockdown check in doConnect() reads the protocol; it must never look like IKEv2.
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));
    QVERIFY(!strategy.isFailed());
}

void TestEmergencyConnectionAttemptStrategy::testEndpointMappingAndIteration()
{
    EmergencyConnectionAttemptStrategy strategy;
    QSignalSpy resolvedSpy(&strategy, &IConnectionAttemptStrategy::hostnamesResolved);
    strategy.onEndpointsReceived({{"10.0.0.1", 443, false}, {"10.0.0.2", 1194, true}});
    QCOMPARE(resolvedSpy.count(), 1);

    CurrentConnectionDescr descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.connectionNodeType, CONNECTION_NODE_DEFAULT);
    QCOMPARE(descr.ip, QString("10.0.0.1"));
    QCOMPARE(descr.port, 443u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_UDP));

    strategy.putFailedConnection();
    QVERIFY(!strategy.isFailed());
    descr = strategy.getCurrentConnectionSettings();
    QCOMPARE(descr.ip, QString("10.0.0.2"));
    QCOMPARE(descr.port, 1194u);
    QCOMPARE(descr.protocol, types::Protocol(types::Protocol::OPENVPN_TCP));

    strategy.putFailedConnection();
    QVERIFY(strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
}

void TestEmergencyConnectionAttemptStrategy::testEmptyEndpointListFails()
{
    EmergencyConnectionAttemptStrategy strategy;
    strategy.onEndpointsReceived({});

    QVERIFY(strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
}

void TestEmergencyConnectionAttemptStrategy::testResolveAfterFetchEmitsFromCache()
{
    EmergencyConnectionAttemptStrategy strategy;
    strategy.onEndpointsReceived({{"10.0.0.1", 443, false}});

    // Subsequent attempts re-resolve; the cached list answers without another fetch.
    QSignalSpy resolvedSpy(&strategy, &IConnectionAttemptStrategy::hostnamesResolved);
    strategy.resolveHostnames();
    QCOMPARE(resolvedSpy.count(), 1);
    QCOMPARE(strategy.getCurrentConnectionSettings().ip, QString("10.0.0.1"));
}

void TestEmergencyConnectionAttemptStrategy::testPreFetchFailureConsumesNoEndpoint()
{
    // A connecting timeout can fail the attempt while the endpoint fetch is still in flight; that
    // failure must not skip an endpoint that was never attempted.
    EmergencyConnectionAttemptStrategy strategy;
    strategy.putFailedConnection();
    strategy.onEndpointsReceived({{"10.0.0.1", 443, false}});

    QCOMPARE(strategy.getCurrentConnectionSettings().ip, QString("10.0.0.1"));
    QVERIFY(!strategy.isFailed());
}

void TestEmergencyConnectionAttemptStrategy::testResetClearsFetchedState()
{
    EmergencyConnectionAttemptStrategy strategy;
    strategy.onEndpointsReceived({{"10.0.0.1", 443, false}});
    strategy.putFailedConnection();

    strategy.reset();

    QVERIFY(!strategy.isFailed());
    QCOMPARE(strategy.getCurrentConnectionSettings().connectionNodeType, CONNECTION_NODE_ERROR);
    // A fresh fetch restarts iteration at the head of the new list.
    strategy.onEndpointsReceived({{"10.0.0.9", 80, true}});
    QCOMPARE(strategy.getCurrentConnectionSettings().ip, QString("10.0.0.9"));
}

void TestEmergencyConnectionAttemptStrategy::testStaticAnswers()
{
    EmergencyConnectionAttemptStrategy strategy;
    QCOMPARE(strategy.preResolveProtocol(), types::Protocol(types::Protocol::OPENVPN_UDP));
    QVERIFY(!strategy.isAutomaticMode());
    QVERIFY(strategy.usesConnectTimeout());
    QVERIFY(!strategy.surfacesTunnelTestFailure());
    QVERIFY(!strategy.hasProtocolChanged());
    QVERIFY(!strategy.shouldWaitForNetwork());
    QVERIFY(strategy.shouldRetryOnAttemptFailure());
}

QTEST_MAIN(TestEmergencyConnectionAttemptStrategy)
