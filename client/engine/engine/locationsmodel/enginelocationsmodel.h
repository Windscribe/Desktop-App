#pragma once

#include <QObject>

#include "apilocationsmodel.h"
#include "customconfiglocationsmodel.h"
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"

namespace locationsmodel {

// Combine ApiLocationsModel and CustomConfigLocationsModel
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager);
    ~LocationsModel() override;

    void setApiLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void setCustomConfigLocations(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated(const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer<QVector<types::Location> > locations);
    void customConfigsLocationsUpdated(QSharedPointer<types::Location > location);
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
