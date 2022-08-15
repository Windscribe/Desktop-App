#include "locationsmodeldata.test.h"
#include "types/locationid.h"

QVector<types::LocationItem> TestData::createBasicTestData()
{
    // basic test model
    // Country1
    //      City1
    //      City2
    // Country2
    //      City1
    //      City2
    //      City3
    // Country3
    // Country4
    //      City1
    // StaticIPS
    //      City1
    //      City2

    QVector<types::LocationItem> locations;
    {
        types::LocationItem l;
        l.name = "Country1";
        l.id = LocationID::createTopApiLocationId(1);
        l.countryCode = "FR";
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 150, false, false, 10000, 50);
        addCity(&l, 2, 55, true, false, 10000, 80);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country2";
        l.id = LocationID::createTopApiLocationId(2);
        l.countryCode = "AU";
        l.isPremiumOnly = true;
        l.isNoP2P = false;

        addCity(&l, 1, 400, false, false, 1000, 50);
        addCity(&l, 2, -2, false, true, 10000, -1);
        addCity(&l, 3, 10, true, false, 10000, 30);
        locations << l;
    }

    {
        // without cities
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(3);
        l.countryCode = "GB";
        l.isPremiumOnly = false;
        l.isNoP2P = true;
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country4";
        l.id = LocationID::createTopApiLocationId(4);
        l.countryCode = "NL";
        l.isPremiumOnly = true;
        l.isNoP2P = true;
        addCity(&l, 1, 400, false, false, 10000, -1);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Static IPS";
        l.id = LocationID::createTopStaticLocationId();
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 120, false, false, 500, 100);
        addCity(&l, 2, 340, false, false, 10000, 80);
        locations << l;
    }

    return locations;
}

QVector<types::LocationItem> TestData::createTestDataWithDeletedCities()
{
    QVector<types::LocationItem> locations;
    {
        types::LocationItem l;
        l.name = "Country1";
        l.id = LocationID::createTopApiLocationId(1);
        l.countryCode = "FR";
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 150, false, false, 10000, 50);
        //addCity(l, 2, 55, true, false, 10000, 80);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country2";
        l.id = LocationID::createTopApiLocationId(2);
        l.countryCode = "AU";
        l.isPremiumOnly = true;
        l.isNoP2P = false;

        //addCity(l, 1, 400, false, false, 1000, 50);
        addCity(&l, 2, -2, false, true, 10000, -1);
        addCity(&l, 3, 10, false, false, 10000, 30);
        locations << l;
    }

    {
        // without cities
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(3);
        l.countryCode = "GB";
        l.isPremiumOnly = false;
        l.isNoP2P = true;
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country4";
        l.id = LocationID::createTopApiLocationId(4);
        l.countryCode = "NL";
        l.isPremiumOnly = true;
        l.isNoP2P = true;
        addCity(&l, 1, 400, false, false, 10000, -1);
        locations << l;
    }
    {
        types::LocationItem l;
        l.name = "Static IPS";
        l.id = LocationID::createTopStaticLocationId();
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 120, false, false, 500, 100);
        addCity(&l, 2, 340, false, false, 10000, 80);
        locations << l;
    }

    return locations;
}

QVector<types::LocationItem> TestData::createTestDataWithDeletedCountries()
{
    QVector<types::LocationItem> locations;

    {
        types::LocationItem l;
        l.name = "Country2";
        l.id = LocationID::createTopApiLocationId(2);
        l.countryCode = "AU";
        l.isPremiumOnly = true;
        l.isNoP2P = false;

        addCity(&l, 1, 400, false, false, 1000, 50);
        addCity(&l, 2, -2, false, true, 10000, -1);
        addCity(&l, 3, 10, false, false, 10000, 30);
        locations << l;
    }

    {
        // without cities
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(3);
        l.countryCode = "GB";
        l.isPremiumOnly = false;
        l.isNoP2P = true;
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country4";
        l.id = LocationID::createTopApiLocationId(4);
        l.countryCode = "NL";
        l.isPremiumOnly = true;
        l.isNoP2P = true;

        addCity(&l, 1, 400, false, false, 10000, -1);
        locations << l;
    }

    return locations;
}

QVector<types::LocationItem> TestData::createTestDataWithChangedCities()
{
    QVector<types::LocationItem> locations;
    {
        types::LocationItem l;
        l.name = "Country1";
        l.id = LocationID::createTopApiLocationId(1);
        l.countryCode = "FR";
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 150, false, false, 10000, 50);
        addCity(&l, 2, 55, true, false, 10000, 80);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(2);
        l.countryCode = "GB";
        l.isPremiumOnly = true;
        l.isNoP2P = false;

        addCity(&l, 1, 23, false, false, 55, 50);
        addCity(&l, 2, 56, false, true, 44, -1);
        addCity(&l, 3, 111, true, false, 10000, 30);
        locations << l;
    }

    {
        // without cities
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(3);
        l.countryCode = "GB";
        l.isPremiumOnly = false;
        l.isNoP2P = true;
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country4";
        l.id = LocationID::createTopApiLocationId(4);
        l.countryCode = "NL";
        l.isPremiumOnly = true;
        l.isNoP2P = true;
        addCity(&l, 1, 555, true, false, 10000, -1);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Static IPS";
        l.id = LocationID::createTopStaticLocationId();
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 120, false, false, 55, 100);
        addCity(&l, 2, 340, false, false, 10000, 50);
        locations << l;
    }
    return locations;
}

QVector<types::LocationItem> TestData::createTestDataWithChangedCities2()
{
    QVector<types::LocationItem> locations;
    {
        types::LocationItem l;
        l.name = "Country1";
        l.id = LocationID::createTopApiLocationId(1);
        l.countryCode = "FR";
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 150, false, false, 10000, 50);
        addCity(&l, 2, 55, true, false, 10000, 80);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country2";
        l.id = LocationID::createTopApiLocationId(2);
        l.countryCode = "AU";
        l.isPremiumOnly = true;
        l.isNoP2P = false;

        addCity(&l, 4, -2, false, true, 10000, -1);
        addCity(&l, 5, -2, false, true, 10000, -1);
        addCity(&l, 1, 12, false, false, 1000, 50);
        addCity(&l, 3, 10, true, false, 10000, 30);
        locations << l;
    }

    {
        // without cities
        types::LocationItem l;
        l.name = "Country3";
        l.id = LocationID::createTopApiLocationId(3);
        l.countryCode = "GB";
        l.isPremiumOnly = false;
        l.isNoP2P = true;
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Country4";
        l.id = LocationID::createTopApiLocationId(4);
        l.countryCode = "NL";
        l.isPremiumOnly = true;
        l.isNoP2P = true;

        addCity(&l, 1, 400, false, false, 10000, -1);
        locations << l;
    }

    {
        types::LocationItem l;
        l.name = "Static IPS";
        l.id = LocationID::createTopStaticLocationId();
        l.isPremiumOnly = false;
        l.isNoP2P = false;

        addCity(&l, 1, 120, false, false, 500, 100);
        addCity(&l, 2, 340, false, false, 10000, 80);
        locations << l;
    }

    return locations;
}

types::LocationItem TestData::createBasicCustomConfigTestData()
{
    types::LocationItem l;
    l.name = "CustomConfigs";
    l.id = LocationID::createTopCustomConfigsLocationId();
    l.isPremiumOnly = false;
    l.isNoP2P = false;

    {
        types::CityItem city;
        city.id = LocationID::createCustomConfigLocationId("aaa.ovpn");
        city.city = "aaa.ovpn";
        city.customConfigType = CUSTOM_CONFIG_OPENVPN;
        city.customConfigIsCorrect = true;
        city.pingTimeMs = -2;
        l.cities << city;
    }
    {
        types::CityItem city;
        city.id = LocationID::createCustomConfigLocationId("bbb.conf");
        city.city = "bbb.conf";
        city.customConfigType = CUSTOM_CONFIG_WIREGUARD;
        city.customConfigIsCorrect = false;
        city.customConfigErrorMessage = "error msg";
        city.pingTimeMs = -2;
        l.cities << city;
    }

    return l;
}

void TestData::addCity(types::LocationItem *l, int num, int pingTime, bool isPremiumOnly, bool isDisabled, int linkSpeed, int health)
{
    types::CityItem city;
    std::string cityName = "City" + std::to_string(num);
    std::string nickName = "Nick" + std::to_string(num);
    if (l->id.isStaticIpsLocation())
    {
        city.id = LocationID::createStaticIpsLocationId(QString::fromStdString(cityName), QString::fromStdString(nickName));
    }
    else
    {
        city.id = LocationID::createApiLocationId(l->id.id(), QString::fromStdString(cityName), QString::fromStdString(nickName));
    }
    city.city = QString::fromStdString(cityName);
    city.nick = QString::fromStdString(nickName);
    city.pingTimeMs = pingTime;
    city.isPro = isPremiumOnly;
    city.isDisabled = isDisabled;
    city.linkSpeed = linkSpeed;
    city.health = health;
    city.staticIpType = "dc";
    l->cities << city;
}
