#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "types/locationitem.h"

// test data for the location model
class TestData
{

public:
    static QVector<types::LocationItem> createBasicTestData();
    static QVector<types::LocationItem> createTestDataWithDeletedCities();
    static QVector<types::LocationItem> createTestDataWithDeletedCountries();
    static QVector<types::LocationItem> createTestDataWithChangedCities();
    static QVector<types::LocationItem> createTestDataWithChangedCities2();
    static types::LocationItem createBasicCustomConfigTestData();

    static void addCity(types::LocationItem *l, int num, int pingTime, bool isPremiumOnly, bool isDisabled, int linkSpeed, int health);
};


#endif // TEST_DATA_H
