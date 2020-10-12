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
#include "hostnamesresolver.h"
#include "apilocationsmodel.h"
#include "customconfiglocationsmodel.h"

namespace locationsmodel {

// Combine ApiLocationsModel and CustomConfigLocationsModel
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager);

    void setApiLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void setCustomConfigLocations(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated(const LocationID &bestLocation, QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void customConfigsLocationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void locationPingTimeChanged(const LocationID &id, locationsmodel::PingTime timeMs);

    void whitelistLocationsIpsChanged(const QStringList &ips);
    void whitelistCustomConfigsIpsChanged(const QStringList &ips);

private:
    ApiLocationsModel *apiLocationsModel_;
    CustomConfigLocationsModel *customConfigLocationsModel_;
    PingHost *pingHost_;

};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_H
