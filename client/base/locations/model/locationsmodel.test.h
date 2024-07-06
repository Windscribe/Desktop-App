#pragma once

#include <QObject>
#include <QTest>
#include <QAbstractItemModelTester>
#include "locationsmodel.h"
#include "proxymodels/cities_proxymodel.h"

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
    void testAddDeleteCountry();
    void testAddDeleteCity();
    void testChangedOrder();
    void testChangedCaptions();
    void testFreeSessionStatusChange();

private:
    QVector<types::Location> testOriginal_;
    LocationID bestLocation_;
    types::Location customConfigLocation_;

    QScopedPointer<gui_locations::LocationsModel> locationsModel_;
    QScopedPointer<QAbstractItemModelTester> tester1_;
    QScopedPointer<gui_locations::CitiesProxyModel> citiesModel_;
    QScopedPointer<QAbstractItemModelTester> tester2_;

    bool isModelsCorrect(const LocationID &bestLocation, const QVector<types::Location> &locations,
                         const types::Location &customConfigLocation);

    bool isLocationsModelEqualTo(const LocationID &bestLocation, const QVector<types::Location> &locations,
                                 const types::Location &customConfigLocation);
    bool isCountryEqual(const QModelIndex &miCountry,  const types::Location &l);
    bool isCityEqual(const QModelIndex &miCity,  const types::City &city);

    bool isCitiesModelEqualTo(const QVector<types::Location> &locations, const types::Location &customConfigLocation);

};
