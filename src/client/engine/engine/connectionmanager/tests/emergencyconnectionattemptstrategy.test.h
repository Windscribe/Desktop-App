#pragma once

#include <QObject>
#include <QTest>

// Unit tests for the deterministic parts of EmergencyConnectionAttemptStrategy: endpoint-list iteration,
// descriptor mapping and reset. The wsnet endpoint fetch itself has no seam and stays
// integration-tested; tests inject plain endpoints through onEndpointsReceived().
class TestEmergencyConnectionAttemptStrategy : public QObject
{
    Q_OBJECT

private slots:
    void testDescrBeforeFetchIsError();
    void testEndpointMappingAndIteration();
    void testEmptyEndpointListFails();
    void testResolveAfterFetchEmitsFromCache();
    void testPreFetchFailureConsumesNoEndpoint();
    void testResetClearsFetchedState();
    void testStaticAnswers();
};
