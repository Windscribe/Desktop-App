#include "serversmodel.h"
#include "nodeselectionalgorithm.h"
#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "utils/utils.h"
#include <QThread>

#include "testpingnodecontroller.h"

/*const int typeIdVectorModelExchangeLocationItem = qRegisterMetaType<QSharedPointer<QVector<ModelExchangeLocationItem> >>("QSharedPointer<QVector<ModelExchangeLocationItem> >");
const int typeIdModelExchangeLocationItem = qRegisterMetaType<ModelExchangeLocationItem>("ModelExchangeLocationItem");

ServersModel::ServersModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager,
                           NodesSpeedRatings *nodesSpeedRating, NodesSpeedStore *nodesSpeedStore) :
    IServersModel(parent), bFreeSessionStatusInitialized_(false),
    bFreeSessionStatus_(true),
    //resolveHostnamesForCustomOvpn_(this),
    customOvpnDnsState_(CUSTOM_OVPN_NO),
    mutex_(QMutex::Recursive)
{
    /*pingManagerThread_ = new QThread(this);
    //pingManager_ = new PingManager(NULL, nodesSpeedRating, nodesSpeedStore, stateController, networkStateManager);
    //connect(pingManager_, SIGNAL(connectionSpeedChanged(LocationID, PingTime)), SIGNAL(connectionSpeedChanged(LocationID, PingTime)));
    //pingManager_->moveToThread(pingManagerThread_);

    connect(pingManagerThread_, SIGNAL(started()), pingManager_, SLOT(init()));
    connect(pingManagerThread_, SIGNAL (finished()), pingManagerThread_, SLOT (deleteLater()));
    connect(pingManagerThread_, SIGNAL (finished()), pingManager_, SLOT (deleteLater()));

    pingManagerThread_->start(QThread::LowPriority);

    connect(&resolveHostnamesForCustomOvpn_, SIGNAL(resolved(QSharedPointer<ServerLocation>)), SLOT(onCustomOvpnDnsResolved(QSharedPointer<ServerLocation>)));*/
    //TestPingNodeController::doTest();
    //NodeSelectionAlgorithm::autoTest();
/*}

ServersModel::~ServersModel()
{
    //pingManagerThread_->exit();
    //pingManagerThread_->wait();
    //clearServers();
}

void ServersModel::updateServers(const QVector<apiinfo::Location> &newServers, bool bSkipCustomOvpnDnsResolution)
{
    QMutexLocker locker(&mutex_);

    // if pingManager not initialized waiting (the likelihood of this is close to zero)
    /*while (!pingManager_->isInitialized())
    {
        QThread::msleep(1);
    }

    // emit signal locationInfoChanged
    Q_FOREACH(QSharedPointer<ServerLocation> sl, newServers)
    {
        // location
        LocationID locationId(sl->getId());
        emit locationInfoChanged(locationId, sl->getNodes(), sl->getDnsHostname());

        // cities
        const QStringList cities = sl->getCities();
        for (const QString &cityName: cities)
        {
            LocationID cityLocationId(sl->getId(), cityName);
            emit locationInfoChanged(cityLocationId, sl->getCityNodes(cityName), sl->getDnsHostname());
        }

        if (!bSkipCustomOvpnDnsResolution && sl->getType() == ServerLocation::SERVER_LOCATION_CUSTOM_CONFIG)
        {
            if (resolveHostnamesForCustomOvpn_.resolve(sl))
            {
                customOvpnDnsState_ = CUSTOM_OVPN_IN_PROGRESS;
            }
            else
            {
                setCustomOvpnConfigIpsToFirewall(*sl);
                customOvpnDnsState_ = CUSTOM_OVPN_FINISHED;
            }
        }
    }

    clearServers();

    QSharedPointer<QVector<ModelExchangeLocationItem> > items = QSharedPointer<QVector<ModelExchangeLocationItem> >(new QVector<ModelExchangeLocationItem>);

    servers_.reserve(newServers.count());
    items->reserve(newServers.count());

    Q_FOREACH(QSharedPointer<ServerLocation> sl, newServers)
    {
        QSharedPointer<ServerLocation> copySl(new ServerLocation);
        *copySl = *sl;
        servers_ << copySl;
        serversMap_[sl->getId()] = copySl;
        items->push_back(serverLocationToModelExchangeLocationItem(*sl));
    }

    emit itemsUpdated(items);
    pingManager_->updateServers(newServers, customOvpnDnsState_ == CUSTOM_OVPN_IN_PROGRESS);*/
/*}

void ServersModel::clear()
{
    /*QMutexLocker locker(&mutex_);
    clearServers();
    QSharedPointer<QVector<ModelExchangeLocationItem> > items = QSharedPointer<QVector<ModelExchangeLocationItem> >(new QVector<ModelExchangeLocationItem>);
    emit itemsUpdated(items);
    pingManager_->clear();*/
/*}

void ServersModel::setSessionStatus(bool bFree)
{
    QMutexLocker locker(&mutex_);

    if (!bFreeSessionStatusInitialized_ || bFree != bFreeSessionStatus_)
    {
        bFreeSessionStatusInitialized_ = true;
        bFreeSessionStatus_ = bFree;
    }
}

PingTime ServersModel::getPingTimeMsForLocation(const LocationID &locationId)
{
    /*QMutexLocker locker(&mutex_);

    if (locationId.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        return getPingTimeMsForCustomOvpnConfig(locationId);
    }
    else
    {
        return pingManager_->getPingTimeMs(locationId);
    }*/
 /*   return PingTime();
}

void ServersModel::getNameAndCountryByLocationId(LocationID &locationId, QString &outName, QString &outCountry)
{
    /*QMutexLocker locker(&mutex_);

    auto it = serversMap_.find(locationId.getId());
    if (it == serversMap_.end())
    {
        Q_ASSERT(false);
    }
    else
    {
        QSharedPointer<ServerLocation> sl = it.value();
        outCountry = sl->getCountryCode();
        if (locationId.getCity().isEmpty())
        {
            outName = sl->getName();
        }
        else
        {
            if (sl->getType() == ServerLocation::SERVER_LOCATION_STATIC)
            {
                QVector<ServerNode> sns = sl->getCityNodes(locationId.getCity());
                if (sns.count() > 0)
                {
                    outName = sns[0].getCityNameForShow();
                }
                else
                {
                    Q_ASSERT(false);
                }
            }
            else
            {
                outName = locationId.getCity();
            }
        }
    }*/
/*}

QSharedPointer<MutableLocationInfo> ServersModel::getMutableLocationInfoById(LocationID locationId)
{
   /* QMutexLocker locker(&mutex_);

    auto it = serversMap_.find(locationId.getId());
    if (it == serversMap_.end())
    {
        return QSharedPointer<MutableLocationInfo>();
    }
    else
    {
        QSharedPointer<ServerLocation> sl = it.value();

        int selectedNode;
        if (locationId.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
        {
            selectedNode = 0;
        }
        else
        {
            selectedNode = pingManager_->getSelectedNode(locationId);
        }

        if (locationId.getCity().isEmpty())
        {
            QSharedPointer<MutableLocationInfo> info(new MutableLocationInfo(locationId, sl->getName(), sl->getNodes(),
                                                                             selectedNode, sl->getDnsHostname()));
            info->connect(this, SIGNAL(locationInfoChanged(LocationID,QVector<ServerNode>,QString)), SLOT(locationChanged(LocationID,QVector<ServerNode>,QString)));
            return info;
        }
        else
        {
            QVector<ServerNode> cityNodes = sl->getCityNodes(locationId.getCity());
            if (!cityNodes.isEmpty())
            {
                QSharedPointer<MutableLocationInfo> info(new MutableLocationInfo(locationId,
                                                                                 sl->getId() == LocationID::STATIC_IPS_LOCATION_ID ? cityNodes[0].getCityNameForShow() : locationId.getCity(),
                                                                                 cityNodes,
                                                                                 selectedNode,
                                                                                 sl->getDnsHostname()));
                info->connect(this, SIGNAL(locationInfoChanged(LocationID,QVector<ServerNode>,QString)), SLOT(locationChanged(LocationID,QVector<ServerNode>,QString)));
                return info;
            }
            else
            {
                return QSharedPointer<MutableLocationInfo>();
            }
        }
    }*/
   /* return 0;
}

// example of location string: NL, Toronto #1, etc
LocationID ServersModel::getLocationIdByName(const QString &location)
{
    /*QMutexLocker locker(&mutex_);

    Q_FOREACH(QSharedPointer<ServerLocation> slm, servers_)
    {
        if (slm->getCountryCode().compare(location, Qt::CaseInsensitive) == 0)
        {
            return LocationID(slm->getId());
        }

        const QStringList cities = slm->getCities();
        for (const QString &cityName: cities)
        {
            if (cityName.compare(location, Qt::CaseInsensitive) == 0)
            {
                return LocationID(slm->getId(), cityName);
            }
        }
    }*/
   /* return LocationID();
}

QStringList ServersModel::getCitiesForLocationId(int locationId)
{
    /*QMutexLocker locker(&mutex_);
    Q_FOREACH(QSharedPointer<ServerLocation> slm, servers_)
    {
        if (slm->getId() == locationId)
        {
            return slm->getCities();
        }
    }*/
  /*  return QStringList();
}

void ServersModel::setProxySettings(const ProxySettings &proxySettings)
{
    /*QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(pingManager_, "setProxySettings", Qt::QueuedConnection, Q_ARG(ProxySettings, proxySettings));*/
/*}

void ServersModel::disableProxy()
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(pingManager_, "disableProxy", Qt::QueuedConnection);
}

void ServersModel::enableProxy()
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(pingManager_, "enableProxy", Qt::QueuedConnection);
}

void ServersModel::onCustomOvpnDnsResolved(QSharedPointer<ServerLocation> location)
{
    /*QMutexLocker locker(&mutex_);
    QVector<QSharedPointer<ServerLocation> > newServers = makeCopyOfServerLocationsVector(servers_);
    if (newServers.count() > 0 && newServers[0]->getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        *newServers[0] = *location;
        customOvpnDnsState_ = CUSTOM_OVPN_FINISHED;
        setCustomOvpnConfigIpsToFirewall(*location);
        updateServers(newServers, true);
    }
    else
    {
        Q_ASSERT(false);
    }*/
/*}

void ServersModel::clearServers()
{
     servers_.clear();
     serversMap_.clear();
}

ModelExchangeLocationItem ServersModel::serverLocationToModelExchangeLocationItem(ServerLocation &sl)
{
    ModelExchangeLocationItem modelLocationExchangeItem;
    /*modelLocationExchangeItem.countryCode = sl.getCountryCode();
    if (sl.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        modelLocationExchangeItem.pingTimeMs = getPingTimeMsForCustomOvpnConfig(LocationID(sl.getId()));
    }
    else
    {
        modelLocationExchangeItem.pingTimeMs = pingManager_->getPingTimeMs(LocationID(sl.getId()));
    }
    modelLocationExchangeItem.id = sl.getId();
    modelLocationExchangeItem.name = sl.getName();
    modelLocationExchangeItem.p2p = sl.getP2P();
    modelLocationExchangeItem.isPremiumOnly = sl.isPremiumOnly();
    modelLocationExchangeItem.isDisabled = false;       //todo: remove this
    modelLocationExchangeItem.forceExpand = true;       //todo: remove this
    modelLocationExchangeItem.staticIpsDeviceName = sl.getStaticIpsDeviceName();

    QStringList cities = sl.getCities();
    Q_FOREACH(const QString &cityName, cities)
    {
        ModelExchangeCityItem modelExchangeCityItem;
        modelExchangeCityItem.cityId = cityName;
        modelExchangeCityItem.cityNameForShow = cityName;

        if (sl.getType() == ServerLocation::SERVER_LOCATION_STATIC)
        {
            QVector<ServerNode> sns = sl.getCityNodes(cityName);
            Q_ASSERT(sns.count() == 1);
            if (sns.count() > 0)
            {
                modelExchangeCityItem.staticIp = sns[0].getStaticIp();
                modelExchangeCityItem.staticIpType = sns[0].getStaticIpType();
                modelExchangeCityItem.cityNameForShow = sns[0].getCityNameForShow();
            }
        }

        if (sl.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
        {
            modelExchangeCityItem.pingTimeMs = getPingTimeMsForCustomOvpnConfig(LocationID(sl.getId(), modelExchangeCityItem.cityId));
        }
        else
        {
            modelExchangeCityItem.pingTimeMs = pingManager_->getPingTimeMs(LocationID(sl.getId(), modelExchangeCityItem.cityId));
        }
        modelExchangeCityItem.bShowPremiumStarOnly = false;
        modelLocationExchangeItem.cities << modelExchangeCityItem;
    }

    Q_FOREACH(const QString &s, sl.getProDataCenters())
    {
        ModelExchangeCityItem modelExchangeCityItem;
        modelExchangeCityItem.cityId = s;
        modelExchangeCityItem.cityNameForShow = s;
        modelExchangeCityItem.pingTimeMs = PingTime::PING_FAILED;
        modelExchangeCityItem.bShowPremiumStarOnly = true;
        modelLocationExchangeItem.cities << modelExchangeCityItem;

        emit connectionSpeedChanged(LocationID(sl.getId(), s), PingTime::PING_FAILED);
    }

    if (sl.getProDataCenters().size() > 0)
    {
        emit connectionSpeedChanged(LocationID(sl.getId()), PingTime::PING_FAILED);
    }

    qSort(modelLocationExchangeItem.cities);*/
/*
    return modelLocationExchangeItem;
}

ServerLocation *ServersModel::findServerLocationById(int id)
{
    auto it = serversMap_.find(id);
    if (it != serversMap_.end())
    {
        return it.value().data();
    }
    else
    {
        return NULL;
    }
}

PingTime ServersModel::getPingTimeMsForCustomOvpnConfig(const LocationID &locationId)
{
   /* Q_ASSERT(locationId.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID);
    Q_ASSERT(customOvpnDnsState_ != CUSTOM_OVPN_NO);

    if (customOvpnDnsState_ == CUSTOM_OVPN_FINISHED)
    {
        return pingManager_->getPingTimeMs(locationId);
    }
    else*/
 /*   {
        return PingTime(PingTime::NO_PING_INFO);
    }
}

void ServersModel::setCustomOvpnConfigIpsToFirewall(ServerLocation &sl)
{
    /*QStringList ips = sl.getAllIps();
    // remove duplicates
    QSet<QString> set = ips.toSet();
    emit customOvpnConfgsIpsChanged(set.toList());*/
/*}

bool operator<(const ModelExchangeCityItem& a, const ModelExchangeCityItem& b)
{
    return a.cityNameForShow < b.cityNameForShow;
}*/
