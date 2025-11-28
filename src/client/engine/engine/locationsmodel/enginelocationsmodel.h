#pragma once

#include <QObject>

#include "apilocationsmodel.h"
#include "customconfiglocationsmodel.h"
#include "api_responses/location.h"
#include "api_responses/staticips.h"

namespace locationsmodel {

// Combine ApiLocationsModel and CustomConfigLocationsModel
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager);
    ~LocationsModel() override;

    void setApiLocations(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps);
    void setCustomConfigLocations(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

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
};

} //namespace locationsmodel
