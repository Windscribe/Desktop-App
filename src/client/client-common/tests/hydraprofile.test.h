#pragma once

#include <QObject>
#include <QTest>

class TestHydraProfile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void testParsesTheCliProfile();
    void testRejectsUnknownFields();
    void testRejectsBadAddressesAndPorts();
    void testRejectsWrongKindVersionAndShape();
    void testRefusesAVersion1Profile();
    void testRejectsAServerAddress();
    void testDatacenterMustBeAHostname();
    void testDatacenterIdIsAPositiveWholeNumber();
    void testRejectsDuplicateEntry();
    void testMatchesTheDatacenterById();
    void testSummaryNamesFrontsByTheServerList();
    void testSaveLoadRemove();
    void testSaveRefusesInvalidAndKeepsPrevious();
    void testLoadSaysWhyAStoredVersion1ProfileIsUnused();
    void testLoadDoesNotValidateAStoredProfileAgain();
    void testRedactsTheEntryForLogs();
    void testAllIsAnOptionalBoolean();
    void testLocationsForPreferences();
};
