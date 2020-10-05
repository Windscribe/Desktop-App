#ifndef LOCATIONSMODEL_LOCATIONITEM_H
#define LOCATIONSMODEL_LOCATIONITEM_H

#include <QString>
#include <QVector>
#include "pingtime.h"
#include "types/locationid.h"
#include "engine/customconfigs/customconfigtype.h"

namespace locationsmodel {

struct CityItem
{
    LocationID id;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro;
    bool isDisabled;

    // specific for static IP location
    QString staticIpCountryCode;
    QString staticIpType;
    QString staticIp;

    // specific for custom config location
    customconfigs::CUSTOM_CONFIG_TYPE customConfigType;
    bool customConfigIsCorrect;
    QString customConfigErrorMessage;

    // set default values
    CityItem() : isPro(false), isDisabled(false), customConfigIsCorrect(false), customConfigType(customconfigs::CUSTOM_CONFIG_OPENVPN) {}
};

struct LocationItem
{
    LocationID id;
    QString name;
    QString countryCode;
    bool isPremiumOnly;
    int p2p;
    QString staticIpsDeviceName;            // used only for static Ips location
    QVector<CityItem> cities;

    // set default values
    LocationItem() : isPremiumOnly(false), p2p(0) {}
};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONITEM_H
