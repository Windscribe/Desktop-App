#ifndef LOCATIONSMODEL_LOCATIONITEM_H
#define LOCATIONSMODEL_LOCATIONITEM_H

#include <QString>
#include <QVector>
#include "pingtime.h"

namespace locationsmodel {

struct CityItem
{
    QString cityId;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro;
    bool isDisabled;
    QString staticIpType;
    QString staticIp;
};

struct LocationItem
{
    int id;
    QString name;
    QString countryCode;
    bool isPremiumOnly;
    int p2p;
    QString staticIpsDeviceName;            // used only for static Ips location
    QVector<CityItem> cities;
};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONITEM_H
