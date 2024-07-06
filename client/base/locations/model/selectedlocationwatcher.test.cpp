#include <QtTest>
#include "locationsmodeldata.test.h"
#include "types/locationid.h"
#include "locations/locationsmodel_roles.h"
#include "locations/model/locationsmodel.h"
#include "locations/model/selectedlocationwatcher.h"

class TestSelectedLocationWatcher : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasic();
    void testChange();

private:
    ProtoTypes::ArrayLocations testLocations_;
    ProtoTypes::LocationId bestLocation_;
    ProtoTypes::Location customConfigLocation_;

    QScopedPointer<gui_locations::LocationsModel> locationsModel_;

};

void TestSelectedLocationWatcher::init()
{
    testLocations_ = TestData::createBasicTestData();
    customConfigLocation_ = TestData::createBasicCustomConfigTestData();

    locationsModel_.reset(new gui_locations::LocationsModel());

    bestLocation_ = LocationID::createApiLocationId(2, "City2", "Nick2").apiLocationToBestLocation().toProtobuf();
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    locationsModel_->updateCustomConfigLocation(customConfigLocation_);

}

void TestSelectedLocationWatcher::cleanup()
{
    testLocations_.clear_locations();
    bestLocation_ = ProtoTypes::LocationId();
}

void TestSelectedLocationWatcher::testBasic()
{
    QScopedPointer<gui_locations::SelectedLocationWatcher> selWatcher(new gui_locations::SelectedLocationWatcher(this, locationsModel_.get()));
    QVERIFY(!selWatcher->isCorrect());

    bool bSuccess = selWatcher->setSelectedLocation(LocationID::createApiLocationId(4, "City1", "Nick25"));
    QVERIFY(!bSuccess);
    QVERIFY(!selWatcher->isCorrect());


    bSuccess = selWatcher->setSelectedLocation(LocationID::createApiLocationId(4, "City1", "Nick1"));
    QVERIFY(bSuccess);
    QVERIFY(selWatcher->isCorrect());

    QVERIFY(selWatcher->locationdId() == LocationID::createApiLocationId(4, "City1", "Nick1"));
    QVERIFY(selWatcher->firstName() == "City1");
    QVERIFY(selWatcher->secondName() == "Nick1");
    QVERIFY(selWatcher->countryCode() == "nl");
    QVERIFY(selWatcher->pingTime() == 400);

}

void TestSelectedLocationWatcher::testChange()
{
    QScopedPointer<gui_locations::SelectedLocationWatcher> selWatcher(new gui_locations::SelectedLocationWatcher(this, locationsModel_.get()));
    selWatcher->setSelectedLocation(LocationID::createApiLocationId(4, "City1", "Nick1"));
    locationsModel_->updateLocations(bestLocation_, TestData::createTestDataWithChangedCities());

    QVERIFY(selWatcher->isCorrect());
    QVERIFY(selWatcher->locationdId() == LocationID::createApiLocationId(4, "City1", "Nick1"));
    QVERIFY(selWatcher->firstName() == "City1");
    QVERIFY(selWatcher->secondName() == "Nick1");
    QVERIFY(selWatcher->countryCode() == "nl");
    QVERIFY(selWatcher->pingTime() == 555);
}


QTEST_MAIN(TestSelectedLocationWatcher)
#include "selectedlocationwatcher.test.moc"
