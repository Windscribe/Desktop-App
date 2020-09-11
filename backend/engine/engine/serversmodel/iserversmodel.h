#ifndef ISERVERSMODEL_H
#define ISERVERSMODEL_H

#include <QObject>
#include <QVector>
//#include "engine/types/apiinfo.h"
#include "engine/types/locationid.h"
#include "mutablelocationinfo.h"
#include "pingtime.h"

struct ModelExchangeCityItem
{
    QString cityId;
    QString cityNameForShow;
    PingTime pingTimeMs;
    bool bShowPremiumStarOnly;
    QString staticIpType;
    QString staticIp;
};

struct ModelExchangeLocationItem
{
    int id;
    QString name;
    QString countryCode;
    bool isPremiumOnly;
    bool isDisabled;
    int p2p;
    PingTime pingTimeMs;
    bool forceExpand;
    QString staticIpsDeviceName;            // used only for static Ips location
    QVector<ModelExchangeCityItem> cities;
};

// interface for servers model, all public functions must be thread safe
class IServersModel : public QObject
{
    Q_OBJECT
public:
    explicit IServersModel(QObject *parent) : QObject(parent) {}
    virtual ~IServersModel() {}

    virtual PingTime getPingTimeMsForLocation(const LocationID &locationId) = 0;
    virtual void getNameAndCountryByLocationId(LocationID &locationId, QString &outName, QString &outCountry) = 0;
    virtual QSharedPointer<MutableLocationInfo> getMutableLocationInfoById(LocationID locationId) = 0;

signals:
    void itemsUpdated(QSharedPointer<QVector<ModelExchangeLocationItem> > items);
    void sessionStatusChanged(bool bFreeSessionStatus);
    void connectionSpeedChanged(LocationID id, PingTime timeMs);
};

#endif // ISERVERSMODEL_H
