#include "customconfiglocationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>

#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "customconfiglocationinfo.h"
#include "nodeselectionalgorithm.h"
#include "engine/dnsresolver/dnsresolver.h"


namespace locationsmodel {

CustomConfigLocationsModel::CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager, PingHost *pingHost) : QObject(parent),
    pingStorage_("pingStorageCustomConfigs"),
    pingIpsController_(this, stateController, networkStateManager, pingHost, "ping_log_custom_configs.txt")
{
    connect(&pingIpsController_, SIGNAL(pingInfoChanged(QString,int, bool)), SLOT(onPingInfoChanged(QString,int, bool)));
    connect(&pingIpsController_, SIGNAL(needIncrementPingIteration()), SLOT(onNeedIncrementPingIteration()));
    pingStorage_.incIteration();

    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void*)), SLOT(onResolved(QString,QHostInfo,void*)));
}

void CustomConfigLocationsModel::setCustomConfigs(const QVector<QSharedPointer<const customconfigs::ICustomConfig> > &customConfigs)
{
    // todo synchronize ping time for two instances of PingIpsController
    // todo: check if configs actually changed
    // todo: dns-resolver cache

    QStringList hostnamesForResolve;
    QVector<PingIpInfo> allIps;
    // fill pingInfos_ array
    pingInfos_.clear();
    for (auto config : customConfigs)
    {
        CustomConfigWithPingInfo cc;
        cc.customConfig = config;

        // fill remotes
        const QStringList hostnames = config->hostnames();
        for (auto hostname : hostnames)
        {
            RemoteItem ri;
            ri.ipOrHostname.ip = hostname;
            ri.isHostname = !IpValidation::instance().isIp(hostname);

            if (!ri.isHostname)
            {
                allIps << PingIpInfo(hostname, PingHost::PING_ICMP);
                ri.ipOrHostname.pingTime = pingStorage_.getNodeSpeed(hostname);
            }
            else
            {
                hostnamesForResolve << hostname;
            }

            cc.remotes << ri;
        }

        pingInfos_ << cc;
    }

    generateLocationsUpdated();

    if (hostnamesForResolve.isEmpty())
    {
        startPingAndWhitelistIps();
    }
    else
    {
        for (const QString &hostname : hostnamesForResolve)
        {
            DnsResolver::instance().lookup(hostname, this);
        }
    }
}

void CustomConfigLocationsModel::clear()
{
    pingInfos_.clear();
    pingIpsController_.updateIps(QVector<PingIpInfo>());
    QSharedPointer<QVector<locationsmodel::LocationItem> > empty(new QVector<locationsmodel::LocationItem>());
    emit locationsUpdated(empty);
}

QSharedPointer<BaseLocationInfo> CustomConfigLocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    Q_ASSERT(locationId.isCustomConfigsLocation());

    for (const CustomConfigWithPingInfo &config : pingInfos_)
    {
        if (LocationID::createCustomConfigLocationId(config.customConfig->filename()) == locationId)
        {
             QSharedPointer<BaseLocationInfo> bli(new CustomConfigLocationInfo(locationId, config.customConfig));
             return bli;
        }
    }

    return NULL;
}

void CustomConfigLocationsModel::onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState)
{
    pingStorage_.setNodePing(ip, timems, isFromDisconnectedState);

    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        if (it->setPingTime(ip, timems))
        {
            emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(it->customConfig->filename()), it->getPing());
        }
    }
}

void CustomConfigLocationsModel::onNeedIncrementPingIteration()
{
    pingStorage_.incIteration();
}

void CustomConfigLocationsModel::onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    if (userPointer == this)
    {
        for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
        {
            for (auto remoteIt = it->remotes.begin(); remoteIt != it->remotes.end(); ++remoteIt)
            {
                if (remoteIt->isHostname && remoteIt->ipOrHostname.ip == hostname)
                {
                    remoteIt->isResolved = true;
                    remoteIt->ips.clear();

                    for (const QHostAddress ha : hostInfo.addresses())
                    {
                        IpItem ipItem;
                        ipItem.ip = ha.toString();
                        ipItem.pingTime = pingStorage_.getNodeSpeed(ipItem.ip);
                        remoteIt->ips << ipItem;

                        emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(it->customConfig->filename()), it->getPing());
                    }
                }
            }
        }
        if (isAllResolved())
        {
            startPingAndWhitelistIps();
        }
    }
}

bool CustomConfigLocationsModel::isAllResolved() const
{
    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        for (auto remoteIt = it->remotes.begin(); remoteIt != it->remotes.end(); ++remoteIt)
        {
            if (remoteIt->isHostname && !remoteIt->isResolved)
            {
                return false;
            }
        }
    }
    return true;
}

void CustomConfigLocationsModel::startPingAndWhitelistIps()
{
    QVector<PingIpInfo> allIps;
    QStringList strListIps;

    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        for (auto remoteIt = it->remotes.begin(); remoteIt != it->remotes.end(); ++remoteIt)
        {
            if (remoteIt->isHostname)
            {
                for (auto ipIt = remoteIt->ips.begin(); ipIt != remoteIt->ips.end(); ++ipIt)
                {
                    strListIps << ipIt->ip;
                    allIps << PingIpInfo(ipIt->ip, PingHost::PING_ICMP);
                }
            }
            else
            {
                strListIps << remoteIt->ipOrHostname.ip;
                allIps << PingIpInfo(remoteIt->ipOrHostname.ip, PingHost::PING_ICMP);
            }
        }
    }
    emit whitelistIpsChanged(strListIps);
    pingIpsController_.updateIps(allIps);
}

void CustomConfigLocationsModel::generateLocationsUpdated()
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

    if (!pingInfos_.isEmpty())
    {
        LocationItem item;

        item.id = LocationID::createTopCustomConfigsLocationId();
        item.name = QObject::tr("Custom Configs");
        item.countryCode = "Custom_Configs";
        item.isPremiumOnly = false;
        item.p2p = 1;

        for (const CustomConfigWithPingInfo &config : pingInfos_)
        {
            CityItem city;
            city.id = LocationID::createCustomConfigLocationId(config.customConfig->filename());
            city.city = config.customConfig->name();
            city.pingTimeMs = config.getPing();
            city.isPro = true;
            city.isDisabled = false;

            city.customConfigType = config.customConfig->type();
            city.customConfigIsCorrect = config.customConfig->isCorrect();
            city.customConfigErrorMessage = config.customConfig->getErrorForIncorrect();
            item.cities << city;
        }
        *items << item;
    }

    emit locationsUpdated(items);
}

PingTime CustomConfigLocationsModel::CustomConfigWithPingInfo::getPing() const
{
    // return first not failed ping
    for (const RemoteItem &ri : remotes)
    {
        if (!ri.isHostname)
        {
            if (ri.ipOrHostname.pingTime != PingTime::PING_FAILED)
            {
                return ri.ipOrHostname.pingTime;
            }
        }
        else
        {
            if (ri.isResolved)
            {
                for (const IpItem &ipItem : ri.ips)
                {
                    if (ipItem.pingTime != PingTime::PING_FAILED)
                    {
                        return ipItem.pingTime;
                    }
                }
            }
            else
            {
                return PingTime::NO_PING_INFO;
            }
        }
    }

    return PingTime::PING_FAILED;
}

bool CustomConfigLocationsModel::CustomConfigWithPingInfo::setPingTime(const QString &ip, PingTime pingTime)
{
    for (auto it = remotes.begin(); it != remotes.end(); ++it)
    {
        if (!it->isHostname)
        {
            if (it->ipOrHostname.ip == ip)
            {
                it->ipOrHostname.pingTime = pingTime;
                return true;
            }
        }
        else
        {
            for (auto ipIt = it->ips.begin(); ipIt != it->ips.end(); ++ipIt)
            {
                if (ipIt->ip == ip)
                {
                    ipIt->pingTime = pingTime;
                    return true;
                }
            }
        }
    }
    return false;
}

} //namespace locationsmodel
