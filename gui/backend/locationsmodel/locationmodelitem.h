#ifndef LOCATIONMODELITEM_H
#define LOCATIONMODELITEM_H

#include <QVector>
#include "../types/pingtime.h"
#include "../types/locationid.h"

struct CityModelItem
{
    int initialInd_;        // ind of item without sorting
    LocationID id;
    QString title1;
    QString title2;
    QString countryCode;
    PingTime pingTimeMs;
    bool bShowPremiumStarOnly;
    bool isFavorite;
    QString staticIpType;
    QString staticIp;
    bool isDisabled;
};

struct LocationModelItem
{
    int initialInd_;        // ind of item without sorting
    LocationID id;
    QString title;
    QString countryCode;
    bool isShowP2P;
    bool isPremiumOnly;
    PingTime pingTimeMs;
    bool isForceExpand;
    QVector<CityModelItem> cities;
};

#endif // LOCATIONMODELITEM_H
