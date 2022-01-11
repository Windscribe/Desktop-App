#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "types/locationid.h"
#include "engine/proxy/proxysettings.h"
#include "locationitem.h"
#include "pingipscontroller.h"
#include "pingstorage.h"
#include "bestlocation.h"
#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"
#include "apilocationsmodel.h"
#include "customconfiglocationsmodel.h"

namespace locationsmodel {

// Combine ApiLocationsModel and CustomConfigLocationsModel
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager);
    ~LocationsModel() override;

    void forceSendLocationsToCli();
    void setApiLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void setCustomConfigLocations(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated(const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void locationsUpdatedCliOnly(const LocationID &bestLocation, QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void customConfigsLocationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void bestLocationUpdated(const LocationID &bestLocation);
    void locationPingTimeChanged(const LocationID &id, PingTime timeMs);

    void whitelistLocationsIpsChanged(const QStringList &ips);
    void whitelistCustomConfigsIpsChanged(const QStringList &ips);

private:
    ApiLocationsModel *apiLocationsModel_;
    CustomConfigLocationsModel *customConfigLocationsModel_;
    PingHost *pingHost_;
    QThread *pingThread_;

};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_H
