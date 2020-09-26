#ifndef LOCATIONSMODEL_LOCATIONITEM_H
#define LOCATIONSMODEL_LOCATIONITEM_H

#include <QString>
#include <QVector>
#include "pingtime.h"
#include "types/locationid.h"

namespace locationsmodel {

struct CityItem
{
    LocationID id;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro;
    bool isDisabled;
    QString staticIpCountryCode;
    QString staticIpType;
    QString staticIp;
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
};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONITEM_H
