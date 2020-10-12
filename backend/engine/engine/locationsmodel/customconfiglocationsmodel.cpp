#include "customconfiglocationsmodel.h"
#include "utils/utils.h"
#include <QFile>
#include <QTextStream>
#include "utils/logger.h"
#include "customconfiglocationinfo.h"
#include "nodeselectionalgorithm.h"

namespace locationsmodel {

CustomConfigLocationsModel::CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager, PingHost *pingHost) : QObject(parent),
    hostnameResolver_(this), pingIpsController_(this, stateController, networkStateManager, pingHost, "ping_log_custom_configs.txt")
{
    connect(&pingIpsController_, SIGNAL(pingInfoChanged(QString,int, bool)), SLOT(onPingInfoChanged(QString,int, bool)));
    connect(&pingIpsController_, SIGNAL(needIncrementPingIteration()), SLOT(onNeedIncrementPingIteration()));
    pingStorage_.incIteration();

    connect(&hostnameResolver_, SIGNAL(allResolved()), SLOT(onHostnamesResolved()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(onTimer()));
    timer->start(1000 * 10);
}

void CustomConfigLocationsModel::setCustomConfigs(const QVector<QSharedPointer<const customconfigs::ICustomConfig> > &customConfigs)
{
    customConfigs_ = customConfigs;
    generateLocationsUpdated();
}

/*void CustomConfigLocationsModel::setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps, const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs)
{
    if (!isChanged(locations, staticIps, customConfigs))
    {
        return;
    }

    hostnameResolver_.clear();

    locations_ = locations;
    staticIps_ = staticIps;
    customConfigs_ = customConfigs;

    whitelistIps();

    // ping stuff
    QVector<PingIpInfo> ips;
    QStringList stringListIps;
    for (const apiinfo::Location &l : locations)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            PingIpInfo pii;
            pii.ip = l.getGroup(i).getPingIp();
            pii.pingType = PingHost::PING_TCP;
            ips << pii;
            stringListIps << pii.ip;
        }
    }

    // handle static ips location
    for (int i = 0; i < staticIps_.getIpsCount(); ++i)
    {
        const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
        PingIpInfo pii;
        pii.ip = sid.getPingIp();
        pii.pingType = PingHost::PING_TCP;
        ips << pii;
        stringListIps << pii.ip;
    }

    // handle custom configs
    QStringList hostnamesForResolve;
    for (auto config : customConfigs_)
    {
        const QStringList hostnames = config->hostnames();
        for (auto hostname : hostnames)
        {
            stringListIps << hostname;
            if (IpValidation::instance().isIp(hostname))
            {
                PingIpInfo pii;
                pii.ip = hostname;
                pii.pingType = PingHost::PING_ICMP;
                ips << pii;
            }
            else
            {
                hostnamesForResolve << hostname;
            }
        }
    }

    pingStorage_.updateNodes(stringListIps);
    pingIpsController_.updateIps(ips, false);

    if (hostnamesForResolve.isEmpty())
    {
        pingIpsController_.updateIps(QVector<PingIpInfo>(), true);      // clear hostnames IPs
    }
    else
    {
        hostnameResolver_.setHostnames(hostnamesForResolve);
    }

    generateLocationsUpdated();
}*/

void CustomConfigLocationsModel::clear()
{
    /*locations_.clear();
    staticIps_ = apiinfo::StaticIps();
    pingIpsController_.updateIps(QVector<PingIpInfo>());
    QSharedPointer<QVector<locationsmodel::LocationItem> > empty(new QVector<locationsmodel::LocationItem>());
    emit locationsUpdated(LocationID(), empty);*/
}

QSharedPointer<BaseLocationInfo> CustomConfigLocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    Q_ASSERT(locationId.isCustomConfigsLocation());

    for (const QSharedPointer<const customconfigs::ICustomConfig> config : customConfigs_)
    {
        if (LocationID::createCustomConfigLocationId(config->filename()) == locationId)
        {
             QSharedPointer<BaseLocationInfo> bli(new CustomConfigLocationInfo(locationId, config));
             return bli;
        }
    }

    /*if (locationId.isStaticIpsLocation())
    {
        if (staticIps_.getIpsCount() > 0)
        {
            for (int i = 0; i < staticIps_.getIpsCount(); ++i)
            {
                const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
                LocationID staticIpLocationId = LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp);

                if (staticIpLocationId == locationId)
                {
                    QVector< QSharedPointer<const BaseNode> > nodes;

                    QStringList ips;
                    ips << sid.nodeIP1 << sid.nodeIP2 << sid.nodeIP3;
                    nodes << QSharedPointer<BaseNode>(new StaticLocationNode(ips, sid.hostname, sid.wgPubKey, sid.wgIp, sid.dnsHostname, sid.username, sid.password, sid.getAllStaticIpIntPorts()));

                    QSharedPointer<BaseLocationInfo> bli(new MutableLocationInfo(locationId, sid.cityName + " - " + sid.staticIp, nodes, 0, ""));
                    return bli;
                }
            }
        }
    }
    else if (locationId.isBestLocation())
    {
        modifiedLocationId = locationId.bestLocationToApiLocation();
    }

    for (const apiinfo::Location &l : locations_)
    {
        if (LocationID::createTopApiLocationId(l.getId()) == modifiedLocationId.toTopLevelLocation())
        {
            for (int i = 0; i < l.groupsCount(); ++i)
            {
                const apiinfo::Group group = l.getGroup(i);
                if (LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()) == modifiedLocationId)
                {
                    QVector< QSharedPointer<const BaseNode> > nodes;
                    for (int n = 0; n < group.getNodesCount(); ++n)
                    {
                        const apiinfo::Node &apiInfoNode = group.getNode(n);
                        QStringList ips;
                        ips << apiInfoNode.getIp(0) << apiInfoNode.getIp(1) << apiInfoNode.getIp(2);
                        nodes << QSharedPointer<const ApiLocationNode>(new ApiLocationNode(ips, apiInfoNode.getHostname(), apiInfoNode.getWeight(), group.getWgPubKey()));
                    }

                    int selectedNode = NodeSelectionAlgorithm::selectRandomNodeBasedOnWeight(nodes);
                    QSharedPointer<BaseLocationInfo> bli(new MutableLocationInfo(modifiedLocationId, group.getCity() + " - " + group.getNick(), nodes, selectedNode, l.getDnsHostName()));
                    return bli;
                }
            }
        }
    }*/

    return NULL;
}

void CustomConfigLocationsModel::onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState)
{
    /*if (bFromHostname)
    {
        hostnameResolver_.setLatencyForIp(ip, timems);
        QString hostname = hostnameResolver_.hostnameForIp(ip);
        if (!hostname.isEmpty())
        {
            PingTime pingTime = hostnameResolver_.getLatencyForHostname(hostname);

            pingStorage_.setNodePing(hostname, pingTime, isFromDisconnectedState);

            for (auto config : customConfigs_)
            {
                const QStringList hostnames = config->hostnames();
                if (hostnames.contains(hostname))
                {
                    emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(config->filename()), pingTime);
                }
            }
        }
    }
    else
    {

        pingStorage_.setNodePing(ip, timems, isFromDisconnectedState);

        bool isAllNodesHaveCurIteration;
        bool isAllNodesInDisconnectedState;
        pingStorage_.getState(isAllNodesHaveCurIteration, isAllNodesInDisconnectedState);

        if (isAllNodesHaveCurIteration)
        {
            detectBestLocation(isAllNodesInDisconnectedState);
        }

        for (const apiinfo::Location &l : locations_)
        {
            for (int i = 0; i < l.groupsCount(); ++i)
            {
                const apiinfo::Group group = l.getGroup(i);
                if (group.getPingIp() == ip)
                {
                    emit locationPingTimeChanged(LocationID::createApiLocationId(l.getId(), group.getCity(), group.getNick()), timems);
                }
            }
        }

        // handle static ips location
        if (staticIps_.getIpsCount() > 0)
        {
            for (int i = 0; i < staticIps_.getIpsCount(); ++i)
            {
                const apiinfo::StaticIpDescr &sid = staticIps_.getIp(i);
                if (sid.getPingIp() == ip)
                {
                    emit locationPingTimeChanged(LocationID::createStaticIpsLocationId(sid.cityName, sid.staticIp), timems);
                }
            }
        }

        // handle custom configs
        for (auto config : customConfigs_)
        {
            const QStringList hostnames = config->hostnames();
            if (hostnames.contains(ip))
            {
                emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(config->filename()), timems);
            }
        }
    }*/
}

void CustomConfigLocationsModel::onNeedIncrementPingIteration()
{
    pingStorage_.incIteration();
    hostnameResolver_.clearLatencyInfo();
}

void CustomConfigLocationsModel::onHostnamesResolved()
{
    /*whitelistIps();

    const QStringList ips = hostnameResolver_.allIps();
    QVector<PingIpInfo> pingInfos;
    for (auto ip : ips)
    {
        PingIpInfo pii;
        pii.ip = ip;
        pii.pingType = PingHost::PING_ICMP;
        pingInfos << pii;
    }
    pingIpsController_.updateIps(pingInfos, true);*/
}

void CustomConfigLocationsModel::onTimer()
{
    if (!customConfigs_.isEmpty())
    {
        for (const QSharedPointer<const customconfigs::ICustomConfig> config : customConfigs_)
        {
            PingTime pt = Utils::generateIntegerRandom(1, 1500);
            emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(config->filename()), pt);
            /*CityItem city;
            city.id = LocationID::createCustomConfigLocationId(config->filename());
            city.city = config->name();
            city.pingTimeMs = PingTime::NO_PING_INFO;       // todo
            //city.pingTimeMs = pingStorage_.getNodeSpeed(sid.getPingIp());
            city.isPro = true;
            city.isDisabled = false;

            city.customConfigType = config->type();
            city.customConfigIsCorrect = config->isCorrect();
            city.customConfigErrorMessage = config->getErrorForIncorrect();
            item.cities << city;*/
        }
    }
}

void CustomConfigLocationsModel::generateLocationsUpdated()
{
    QSharedPointer <QVector<LocationItem> > items(new QVector<LocationItem>());

    if (!customConfigs_.isEmpty())
    {
        LocationItem item;

        item.id = LocationID::createTopCustomConfigsLocationId();
        item.name = QObject::tr("Custom Configs");
        item.countryCode = "Custom_Configs";
        item.isPremiumOnly = false;
        item.p2p = 1;

        for (const QSharedPointer<const customconfigs::ICustomConfig> config : customConfigs_)
        {
            CityItem city;
            city.id = LocationID::createCustomConfigLocationId(config->filename());
            city.city = config->name();
            city.pingTimeMs = PingTime::NO_PING_INFO;       // todo
            //city.pingTimeMs = pingStorage_.getNodeSpeed(sid.getPingIp());
            city.isPro = true;
            city.isDisabled = false;

            city.customConfigType = config->type();
            city.customConfigIsCorrect = config->isCorrect();
            city.customConfigErrorMessage = config->getErrorForIncorrect();
            item.cities << city;
        }
        *items << item;
    }

    emit locationsUpdated(items);
}

void CustomConfigLocationsModel::whitelistIps()
{
    /*QStringList ips;
    for (const apiinfo::Location &l : locations_)
    {
        for (int i = 0; i < l.groupsCount(); ++i)
        {
            ips << l.getGroup(i).getAllIps();
        }
    }
    ips << staticIps_.getAllIps();
    ips << hostnameResolver_.allIps();
    emit whitelistIpsChanged(ips);*/
}

bool CustomConfigLocationsModel::isChanged(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps, const QVector<QSharedPointer<const customconfigs::ICustomConfig> > &customConfigs)
{
    /*bool bCustomConfigsChanged = false;
    if (customConfigs_.size() != customConfigs.size())
    {
        bCustomConfigsChanged = true;
    }
    else
    {
        for (int i = 0; i < customConfigs_.size(); ++i)
        {
            if (customConfigs_[i]->filename() != customConfigs[i]->filename())
            {
                bCustomConfigsChanged = true;
                break;
            }
        }
    }

    return bCustomConfigsChanged || locations_ != locations || staticIps_ != staticIps;*/
    return true;
}

} //namespace locationsmodel
