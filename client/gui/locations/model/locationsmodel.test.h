#ifndef TEST_LOCATIONSMODEL_H
#define TEST_LOCATIONSMODEL_H

#include <QObject>
#include <QTest>
#include <QAbstractItemModelTester>
#include "locationsmodel.h"
#include "citiesmodel.h"

// tests for classes LocationsModel and CitiesModel
// CitiesModel depends on class LocationsModel data so it makes sense to test them together
class TestLocationsModel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testBasic();
    void testBestLocation();
    void testCustomConfig();
    void testConnectionSpeed();
    void testAddCountry();
    void testAddCity();
    void testDeleteCity();
    void testDeleteCountry();
    void testChangedCities();
    void testChangedCities2();
    void testFreeSessionStatusChange();

private:
    QVector<types::LocationItem> testLocations_;
    LocationID bestLocation_;
    types::LocationItem customConfigLocation_;

    QScopedPointer<gui::LocationsModel> locationsModel_;
    QScopedPointer<QAbstractItemModelTester> tester1_;
    QScopedPointer<gui::CitiesModel> citiesModel_;
    QScopedPointer<QAbstractItemModelTester> tester2_;

    bool isModelsCorrect(const LocationID &bestLocation, const QVector<types::LocationItem> &locations,
                         const types::LocationItem &customConfigLocation);

    bool isLocationsModelEqualTo(const LocationID &bestLocation, const QVector<types::LocationItem> &locations,
                                 const types::LocationItem &customConfigLocation);
    bool isCountryEqual(const QModelIndex &miCountry,  const types::LocationItem &l);
    bool isCityEqual(const QModelIndex &miCity,  const types::CityItem &city);

    bool isCitiesModelEqualTo(const QVector<types::LocationItem> &locations, const types::LocationItem &customConfigLocation);

};


#endif // TEST_LOCATIONSMODEL_H
