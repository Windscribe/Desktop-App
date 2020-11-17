#include "locationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>
#include <QThread>
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "mutablelocationinfo.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

LocationsModel::LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager) : QObject(parent)
{
    pingThread_ = new QThread(this);
    pingHost_ = new PingHost(nullptr, stateController);
    pingHost_->moveToThread(pingThread_);
    pingThread_->start(QThread::HighPriority);

    apiLocationsModel_ = new ApiLocationsModel(this, stateController, networkStateManager, pingHost_);
    customConfigLocationsModel_ = new CustomConfigLocationsModel(this, stateController, networkStateManager, pingHost_);

    connect(apiLocationsModel_, SIGNAL(locationsUpdated(LocationID,QSharedPointer<QVector<locationsmodel::LocationItem> >)), SIGNAL(locationsUpdated(LocationID,QSharedPointer<QVector<locationsmodel::LocationItem> >)));
    connect(apiLocationsModel_, SIGNAL(bestLocationUpdated(LocationID)), SIGNAL(bestLocationUpdated(LocationID)));
    connect(apiLocationsModel_, SIGNAL(locationPingTimeChanged(LocationID,locationsmodel::PingTime)), SIGNAL(locationPingTimeChanged(LocationID,locationsmodel::PingTime)));
    connect(apiLocationsModel_, SIGNAL(whitelistIpsChanged(QStringList)), SIGNAL(whitelistLocationsIpsChanged(QStringList)));

    connect(customConfigLocationsModel_, SIGNAL(locationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> >)), SIGNAL(customConfigsLocationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> >)));
    connect(customConfigLocationsModel_, SIGNAL(locationPingTimeChanged(LocationID,locationsmodel::PingTime)), SIGNAL(locationPingTimeChanged(LocationID,locationsmodel::PingTime)));
    connect(customConfigLocationsModel_, SIGNAL(whitelistIpsChanged(QStringList)), SIGNAL(whitelistCustomConfigsIpsChanged(QStringList)));
}

LocationsModel::~LocationsModel()
{
    pingThread_->quit();
    pingThread_->wait();
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

void LocationsModel::setProxySettings(const ProxySettings &proxySettings)
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
    if (locationId.isCustomConfigsLocation())
    {
        return customConfigLocationsModel_->getMutableLocationInfoById(locationId);
    }
    else
    {
        return apiLocationsModel_->getMutableLocationInfoById(locationId);
    }
}

} //namespace locationsmodel
