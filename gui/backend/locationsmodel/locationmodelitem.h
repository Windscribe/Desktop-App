#ifndef LOCATIONMODELITEM_H
#define LOCATIONMODELITEM_H

#include <QVector>
#include "../types/pingtime.h"
#include "../types/locationid.h"

//todo: implicit sharing to avoid copy constructors
struct CityModelItem
{
    LocationID id;
    QString title1;
    QString title2;
    QString countryCode;        // maybe remove
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
    QVector<CityModelItem> cities;


    qint32 calcAveragePing() const
    {
        qint32 averagePing = 0;
        for (const CityModelItem &cmi : cities)
        {
            if (cmi.pingTimeMs == PingTime::NO_PING_INFO)
            {
                averagePing += 0;
            }
            else if (cmi.pingTimeMs == PingTime::PING_FAILED)
            {
                averagePing += 2000;    // 2000 - max ping interval
            }
            else
            {
                averagePing += cmi.pingTimeMs.toInt();
            }
        }
        return averagePing / cities.count();
    }

};

#endif // LOCATIONMODELITEM_H
