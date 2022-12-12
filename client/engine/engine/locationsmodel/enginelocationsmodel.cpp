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

    connect(apiLocationsModel_, SIGNAL(locationsUpdated(LocationID, QString, QSharedPointer<QVector<types::Location> >)), SIGNAL(locationsUpdated(LocationID, QString, QSharedPointer<QVector<types::Location> >)));
    connect(apiLocationsModel_, SIGNAL(bestLocationUpdated(LocationID)), SIGNAL(bestLocationUpdated(LocationID)));
    connect(apiLocationsModel_, SIGNAL(locationPingTimeChanged(LocationID,PingTime)), SIGNAL(locationPingTimeChanged(LocationID,PingTime)));
    connect(apiLocationsModel_, SIGNAL(whitelistIpsChanged(QStringList)), SIGNAL(whitelistLocationsIpsChanged(QStringList)));

    connect(customConfigLocationsModel_, SIGNAL(locationsUpdated(QSharedPointer<types::Location>)), SIGNAL(customConfigsLocationsUpdated(QSharedPointer<types::Location>)));
    connect(customConfigLocationsModel_, SIGNAL(locationPingTimeChanged(LocationID,PingTime)), SIGNAL(locationPingTimeChanged(LocationID,PingTime)));
    connect(customConfigLocationsModel_, SIGNAL(whitelistIpsChanged(QStringList)), SIGNAL(whitelistCustomConfigsIpsChanged(QStringList)));
}

LocationsModel::~LocationsModel()
{
    pingThread_->quit();
    pingThread_->wait();
    delete pingHost_;
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
