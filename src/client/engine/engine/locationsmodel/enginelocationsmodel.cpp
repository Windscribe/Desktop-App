#include "enginelocationsmodel.h"

#include <QThread>

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent)
{
    apiLocationsModel_ = new ApiLocationsModel(this, stateController, networkDetectionManager);
    customConfigLocationsModel_ = new CustomConfigLocationsModel(this, stateController, networkDetectionManager);

    connect(apiLocationsModel_, &ApiLocationsModel::locationsUpdated, this, &LocationsModel::locationsUpdated);
    connect(apiLocationsModel_, &ApiLocationsModel::bestLocationUpdated, this, &LocationsModel::bestLocationUpdated);
    connect(apiLocationsModel_, &ApiLocationsModel::locationPingTimeChanged, this, &LocationsModel::locationPingTimeChanged);
    connect(apiLocationsModel_, &ApiLocationsModel::whitelistIpsChanged, this, &LocationsModel::whitelistLocationsIpsChanged);

    connect(customConfigLocationsModel_, &CustomConfigLocationsModel::locationsUpdated, this, &LocationsModel::customConfigsLocationsUpdated);
    connect(customConfigLocationsModel_, &CustomConfigLocationsModel::locationPingTimeChanged, this, &LocationsModel::locationPingTimeChanged);
    connect(customConfigLocationsModel_, &CustomConfigLocationsModel::whitelistIpsChanged, this, &LocationsModel::whitelistCustomConfigsIpsChanged);
}

LocationsModel::~LocationsModel()
{
    delete customConfigLocationsModel_;
    delete apiLocationsModel_;
}

void LocationsModel::setApiLocations(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps)
{
    apiLocationsModel_->setLocations(locations, staticIps);
}

void LocationsModel::setCustomConfigLocations(const QVector<QSharedPointer<const customconfigs::ICustomConfig> > &customConfigs)
{
    customConfigLocationsModel_->setCustomConfigs(customConfigs);
}

void LocationsModel::clear()
{
    apiLocationsModel_->clear();
    customConfigLocationsModel_->clear();
}

QSharedPointer<BaseLocationInfo> LocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    if (locationId.isCustomConfigsLocation()) {
        return customConfigLocationsModel_->getMutableLocationInfoById(locationId);
    }
    else {
        return apiLocationsModel_->getMutableLocationInfoById(locationId);
    }
}

} //namespace locationsmodel
