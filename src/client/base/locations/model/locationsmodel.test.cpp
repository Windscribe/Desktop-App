#include <QtTest>
#include "locationsmodel.test.h"
#include "types/locationid.h"
#include "locations/locationsmodel_roles.h"

void TestLocationsModel::init()
{
    {
        QFile file(":data/tests/locationsmodel/original.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        testOriginal_ = types::Location::loadLocationsFromJson(arr);
        QVERIFY(!testOriginal_.isEmpty());
    }
    {
        QFile file(":data/tests/locationsmodel/custom_config.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        customConfigLocation_ = types::Location::loadLocationFromJson(arr);
    }

    locationsModel_.reset(new gui_locations::LocationsModel());
    tester1_.reset(new QAbstractItemModelTester(locationsModel_.get(), QAbstractItemModelTester::FailureReportingMode::QtTest, this));

    citiesModel_.reset(new gui_locations::CitiesProxyModel());
    tester2_.reset(new QAbstractItemModelTester(citiesModel_.get(), QAbstractItemModelTester::FailureReportingMode::QtTest, this));
    citiesModel_->setSourceModel(locationsModel_.get());

    locationsModel_->updateLocations(bestLocation_, testOriginal_);
    locationsModel_->updateCustomConfigLocation(customConfigLocation_);
}

void TestLocationsModel::cleanup()
{
    testOriginal_.clear();
    bestLocation_ = LocationID();
}

void TestLocationsModel::testBasic()
{
    QVERIFY(locationsModel_->columnCount() == 1);
    QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(63));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui_locations::kIsShowAsPremium).toBool() == false);
        QVERIFY(ind.data(gui_locations::kIs10Gbps).toBool() == false);
        QVERIFY(ind.data(gui_locations::kLoad).toInt() == 14);
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 219);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(112, "Lima", "Amaru"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui_locations::kIsShowAsPremium).toBool() == false);
        QVERIFY(ind.data(gui_locations::kIs10Gbps).toBool() == true);
        QVERIFY(ind.data(gui_locations::kLoad).toInt() == 3);
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 257);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(56));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui_locations::kLoad).toInt() == 8);
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 231);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(5));
        QVERIFY(!ind.isValid());
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createStaticIpsLocationId("Toronto", "206.223.183.201"));
        QVERIFY(ind.isValid());
    }
}

void TestLocationsModel::testBestLocation()
{
    {
        QSignalSpy spyChanged(locationsModel_.get(), &QAbstractItemModel::dataChanged);
        QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);

        bestLocation_ = LocationID::createApiLocationId(63, "Vancouver", "Vansterdam").apiLocationToBestLocation();
        locationsModel_->updateBestLocation(bestLocation_);
        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);

        QCOMPARE(spyChanged.count(), 0);
        QCOMPARE(spyRemoved.count(), 0);
        QCOMPARE(spyInserted.count(), 1);
        QList<QVariant> arguments = spyInserted.takeFirst();

        QModelIndex newItem = locationsModel_->index(arguments.at(1).toInt(), 0, arguments.at(0).toModelIndex());
        QVERIFY(arguments.at(1).toInt() == arguments.at(2).toInt());


        QModelIndex indBestLocation = locationsModel_->getIndexByLocationId(bestLocation_);
        QVERIFY(indBestLocation == newItem);
        QVERIFY(indBestLocation.isValid());
        QModelIndex indBestLocation2 = locationsModel_->getBestLocationIndex();
        QVERIFY(indBestLocation == indBestLocation2);

        QVERIFY(indBestLocation.data(gui_locations::kName).toString() == "Best Location");
        QVERIFY(indBestLocation.data(gui_locations::kCountryCode).toString() == "ca");
        QVERIFY(indBestLocation.data(gui_locations::kIs10Gbps).toBool() == true);
        QVERIFY(qvariant_cast<LocationID>(indBestLocation.data(gui_locations::kLocationId)) == bestLocation_);
    }

    {
        QSignalSpy spyChanged(locationsModel_.get(), &QAbstractItemModel::dataChanged);
        QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);

        bestLocation_ = LocationID::createApiLocationId(1, "City3", "Nick3").apiLocationToBestLocation();
        locationsModel_->updateBestLocation(bestLocation_);

        QCOMPARE(spyChanged.count(), 0);
        QCOMPARE(spyRemoved.count(), 1);
        QCOMPARE(spyInserted.count(), 0);

        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == false);

        QModelIndex indBestLocation = locationsModel_->getIndexByLocationId(bestLocation_);
        QVERIFY(!indBestLocation.isValid());
        QModelIndex indBestLocation2 = locationsModel_->getBestLocationIndex();
        QVERIFY(!indBestLocation2.isValid());
    }
}

void TestLocationsModel::testCustomConfig()
{
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("wg0.conf"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui_locations::kCustomConfigType).toString() == "wg");
        QVERIFY(ind.data(gui_locations::kIsCustomConfigCorrect).toBool() == true);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("test.conf"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui_locations::kCustomConfigType).toString() == "wg");
        QVERIFY(ind.data(gui_locations::kIsCustomConfigCorrect).toBool() == false);
        QVERIFY(ind.data(gui_locations::kCustomConfigErrorMessage).toString() == "Missing \"Address\" in the \"Interface\" section");
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("bbb2.conf"));
        QVERIFY(!ind.isValid());
    }
}

void TestLocationsModel::testConnectionSpeed()
{
    bestLocation_ = LocationID::createApiLocationId(65, "Dallas", "BBQ").apiLocationToBestLocation();
    locationsModel_->updateBestLocation(bestLocation_);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(65));
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 164);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(65, "Atlanta", "Piedmont"));
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 173);
    }
    {
        QModelIndex ind = locationsModel_->getBestLocationIndex();
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 176);
    }

    QSignalSpy spyChanged(locationsModel_.get(), &QAbstractItemModel::dataChanged);
    QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
    QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);
    locationsModel_->changeConnectionSpeed(LocationID::createApiLocationId(65, "Dallas", "BBQ"), 500);
    QCOMPARE(spyChanged.count(), 3);
    QCOMPARE(spyRemoved.count(), 0);
    QCOMPARE(spyInserted.count(), 0);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(65));
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 164);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(65, "Dallas", "BBQ"));
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 500);
    }
    {
        QModelIndex ind = locationsModel_->getBestLocationIndex();
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 500);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("server.ovpn"));
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == -1);
        locationsModel_->changeConnectionSpeed(LocationID::createCustomConfigLocationId("server.ovpn"), 1500);
        QVERIFY(ind.data(gui_locations::kPingTime).toInt() == 1500);
    }
}

void TestLocationsModel::testAddDeleteCountry()
{
    QFile file(":data/tests/locationsmodel/deleted_locations.json");
    file.open(QIODevice::ReadOnly);
    QVERIFY(file.isOpen());
    QByteArray arr = file.readAll();
    QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

    locationsModel_->updateLocations(bestLocation_, changed);
    QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

    locationsModel_->updateLocations(bestLocation_, testOriginal_);
    QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);
}

void TestLocationsModel::testAddDeleteCity()
{
    QFile file(":data/tests/locationsmodel/deleted_cities.json");
    file.open(QIODevice::ReadOnly);
    QVERIFY(file.isOpen());
    QByteArray arr = file.readAll();
    QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

    locationsModel_->updateLocations(bestLocation_, changed);
    QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

    locationsModel_->updateLocations(bestLocation_, testOriginal_);
    QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);
}

void TestLocationsModel::testChangedOrder()
{
    {
        QFile file(":data/tests/locationsmodel/changed_locations_order.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

        QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);

        locationsModel_->updateLocations(bestLocation_, changed);
        QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

        locationsModel_->updateLocations(bestLocation_, testOriginal_);
        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);

        QCOMPARE(spyRemoved.count(), 0);
        QCOMPARE(spyInserted.count(), 0);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_cities_order.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

        QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);

        locationsModel_->updateLocations(bestLocation_, changed);
        QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

        locationsModel_->updateLocations(bestLocation_, testOriginal_);
        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);

        QCOMPARE(spyRemoved.count(), 0);
        QCOMPARE(spyInserted.count(), 0);
    }
}

void TestLocationsModel::testChangedCaptions()
{
    {
        QFile file(":data/tests/locationsmodel/changed_locations_captions.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

        QSignalSpy spyRemoved(locationsModel_.get(), &QAbstractItemModel::rowsRemoved);
        QSignalSpy spyInserted(locationsModel_.get(), &QAbstractItemModel::rowsInserted);

        locationsModel_->updateLocations(bestLocation_, changed);
        QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

        locationsModel_->updateLocations(bestLocation_, testOriginal_);
        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);

        QCOMPARE(spyRemoved.count(), 0);
        QCOMPARE(spyInserted.count(), 0);
    }
    {
        QFile file(":data/tests/locationsmodel/changed_cities_captions.json");
        file.open(QIODevice::ReadOnly);
        QVERIFY(file.isOpen());
        QByteArray arr = file.readAll();
        QVector<types::Location> changed = types::Location::loadLocationsFromJson(arr);

        locationsModel_->updateLocations(bestLocation_, changed);
        QVERIFY(isModelsCorrect(bestLocation_, changed, customConfigLocation_) == true);

        locationsModel_->updateLocations(bestLocation_, testOriginal_);
        QVERIFY(isModelsCorrect(bestLocation_, testOriginal_, customConfigLocation_) == true);
    }
}

void TestLocationsModel::testFreeSessionStatusChange()
{
    QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(63, "Vancouver", "Granville"));
    QVERIFY(ind.data(gui_locations::kIsShowAsPremium).toBool() == false);
    locationsModel_->setFreeSessionStatus(true);
    QVERIFY(ind.data(gui_locations::kIsShowAsPremium).toBool() == true);
}

bool TestLocationsModel::isModelsCorrect(const LocationID &bestLocation, const QVector<types::Location> &locations, const types::Location &customConfigLocation)
{
    return isLocationsModelEqualTo(bestLocation, locations, customConfigLocation) &&
           isCitiesModelEqualTo(locations, customConfigLocation);
}

bool TestLocationsModel::isLocationsModelEqualTo(const LocationID &bestLocation, const QVector<types::Location> &locations,
                                                 const types::Location &customConfigLocation)
{
    QSet<int> foundIndexes;
    LocationID foundBestLocation;
    int handledCount = 0;

    for (int i = 0; i < locationsModel_->rowCount(); ++i)
    {
        QModelIndex miCountry = locationsModel_->index(i, 0);
        LocationID lid = qvariant_cast<LocationID>(miCountry.data(gui_locations::kLocationId));
        if (lid.isBestLocation())
        {
            foundBestLocation = lid;
            handledCount++;
        }
        else if (lid.isCustomConfigsLocation())
        {
            if (!isCountryEqual(miCountry,customConfigLocation))
            {
                return false;
            }
            handledCount++;
        }
        else
        {
            for (int l = 0; l < locations.size(); ++l)
            {
                if (isCountryEqual(miCountry,locations[l]))
                {
                    foundIndexes.insert(l);
                    handledCount++;
                    break;
                }
            }

        }
    }
    if (handledCount != locationsModel_->rowCount()) return false;
    return foundBestLocation == bestLocation && foundIndexes.size() == locations.size();
}

bool TestLocationsModel::isCountryEqual(const QModelIndex &miCountry, const types::Location &l)
{
    // Some fields are skipped for compare
    if (miCountry.data(gui_locations::kName).toString() !=  l.name) return false;
    if (qvariant_cast<LocationID>(miCountry.data(gui_locations::kLocationId)) != l.id) return false;
    if (miCountry.data(gui_locations::kCountryCode).toString() !=  l.countryCode.toLower()) return false;
    if (miCountry.data(gui_locations::kIsShowP2P).toBool() !=  l.isNoP2P) return false;

    int citiesCount = miCountry.model()->rowCount(miCountry);
    if (citiesCount != l.cities.size()) return false;

    QSet<int> foundCitiesIndexes;
    int equalCitiesCount = 0;

    for (int i = 0; i < citiesCount; ++i)
    {
        QModelIndex miCity = miCountry.model()->index(i, 0, miCountry);
        for (int k = 0; k < l.cities.size(); k++)
        {
            if (isCityEqual(miCity, l.cities[k]))
            {
                foundCitiesIndexes.insert(k);
                equalCitiesCount++;
                break;
            }
        }
    }

    return equalCitiesCount == citiesCount && foundCitiesIndexes.size() == citiesCount;
}

bool TestLocationsModel::isCityEqual(const QModelIndex &miCity, const types::City &city)
{
    // Some fields are skipped for compare
    if (miCity.data(gui_locations::kName).toString() !=  city.city) return false;
    if (city.id.isStaticIpsLocation())
    {
        if (miCity.data(gui_locations::kNick).toString() !=  city.staticIp) return false;
    }
    else
    {
        if (miCity.data(gui_locations::kNick).toString() !=  city.nick) return false;
    }
    if (qvariant_cast<LocationID>(miCity.data(gui_locations::kLocationId)) != city.id) return false;
    if (miCity.data(gui_locations::kPingTime).toInt() !=  city.pingTimeMs.toInt()) return false;
    if (miCity.data(gui_locations::kIsDisabled).toBool() !=  city.isDisabled) return false;

    if (miCity.data(gui_locations::kStaticIp).toString() != city.staticIp) return false;
    if (miCity.data(gui_locations::kStaticIpType).toString() !=  city.staticIpType) return false;

    if (miCity.data(gui_locations::kIsCustomConfigCorrect).toBool() != city.customConfigIsCorrect) return false;
    if (miCity.data(gui_locations::kCustomConfigErrorMessage).toString() !=  city.customConfigErrorMessage) return false;

    if (miCity.data(gui_locations::kIs10Gbps).toBool() !=  city.is10Gbps) return false;

    return true;
}

bool TestLocationsModel::isCitiesModelEqualTo(const QVector<types::Location> &locations, const types::Location &customConfigLocation)
{
    QVector<types::City> cities;
    int handledCount = 0;
    for (int l = 0; l < locations.size(); ++l)
    {
        for (int c = 0; c < locations[l].cities.size(); ++c)
        {
            cities << locations[l].cities[c];
        }
    }

    for (int c = 0; c < customConfigLocation.cities.size(); ++c)
    {
        cities << customConfigLocation.cities[c];
    }

    for (int i = 0; i < citiesModel_->rowCount(); ++i)
    {
        QModelIndex miCity = citiesModel_->index(i, 0);

        for (int c = 0; c < cities.size(); ++c)
        {
            if (isCityEqual(miCity, cities[c]))
            {
                handledCount++;
                break;
            }
        }
    }
    return handledCount == cities.size();
}

QTEST_MAIN(TestLocationsModel)


