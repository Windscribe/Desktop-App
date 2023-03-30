#include "enginelocationsmodel.h"

#include <QThread>

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent)
{
    pingThread_ = new QThread(this);
    pingHost_ = new PingHost(nullptr, stateController);
    pingHost_->moveToThread(pingThread_);
    connect(pingThread_, &QThread::started, pingHost_, &PingHost::init);
    connect(pingThread_, &QThread::finished, pingHost_, &PingHost::finish);
    pingThread_->start(QThread::HighPriority);

    apiLocationsModel_ = new ApiLocationsModel(this, stateController, networkDetectionManager, pingHost_);
    customConfigLocationsModel_ = new CustomConfigLocationsModel(this, stateController, networkDetectionManager, pingHost_);

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
    pingThread_->quit();
    pingThread_->wait();
    pingHost_->deleteLater();
}

void LocationsModel::setApiLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps)
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

void LocationsModel::setProxySettings(const types::ProxySettings &proxySettings)
{
    pingHost_->setProxySettings(proxySettings);
}

void LocationsModel::disableProxy()
{
    pingHost_->disableProxy();
}

void LocationsModel::enableProxy()
{
    pingHost_->enableProxy();
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
