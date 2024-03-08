#include "customconfiglocationsmodel.h"

#include <QFile>
#include <QTextStream>

#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/ipvalidation.h"
#include "customconfiglocationinfo.h"

namespace locationsmodel {

using namespace wsnet;

CustomConfigLocationsModel::CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    pingManager_(this, stateController, networkDetectionManager, "pingStorageCustomConfigs", "ping_log_custom_configs.txt")
{
    connect(&pingManager_, &PingManager::pingInfoChanged, this, &CustomConfigLocationsModel::onPingInfoChanged);
}

void CustomConfigLocationsModel::setCustomConfigs(const QVector<QSharedPointer<const customconfigs::ICustomConfig> > &customConfigs)
{
    // todo synchronize ping time for two instances of PingIpsController
    // todo: check if configs actually changed
    // todo: dns-resolver cache

    QStringList hostnamesForResolve;
    // fill pingInfos_ array
    pingInfos_.clear();
    for (const auto &config : customConfigs)
    {
        CustomConfigWithPingInfo cc;
        cc.customConfig = config;

        // fill remotes
        const QStringList hostnames = config->hostnames();
        for (const auto &hostname : hostnames)
        {
            RemoteItem ri;
            ri.ipOrHostname.ip = hostname;
            ri.isHostname = !IpValidation::isIp(hostname);

            if (!ri.isHostname)
            {
                ri.ipOrHostname.pingTime = pingManager_.getPing(hostname);
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

    auto callback = [this] (std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
    {
        QMetaObject::invokeMethod(this, [this, hostname, result] {
            onDnsRequestFinished(QString::fromStdString(hostname), result);
        });

    };

    if (hostnamesForResolve.isEmpty())
    {
        startPingAndWhitelistIps();
    }
    else
    {
        for (const QString &hostname : hostnamesForResolve)
        {
            WSNet::instance()->dnsResolver()->lookup(hostname.toStdString(), 0, callback);
        }
    }
}

void CustomConfigLocationsModel::clear()
{
    pingInfos_.clear();
    pingManager_.clearIps();
    QSharedPointer<types::Location> empty(new types::Location());
    emit locationsUpdated(empty);
}

QSharedPointer<BaseLocationInfo> CustomConfigLocationsModel::getMutableLocationInfoById(const LocationID &locationId)
{
    WS_ASSERT(locationId.isCustomConfigsLocation());

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

void CustomConfigLocationsModel::onPingInfoChanged(const QString &ip, int timems)
{
    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        if (it->setPingTime(ip, timems))
        {
            emit locationPingTimeChanged(LocationID::createCustomConfigLocationId(it->customConfig->filename()), it->getPing());
        }
    }
}

void CustomConfigLocationsModel::onDnsRequestFinished(const QString &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result)
{
    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        for (auto remoteIt = it->remotes.begin(); remoteIt != it->remotes.end(); ++remoteIt)
        {
            if (remoteIt->isHostname && remoteIt->ipOrHostname.ip == hostname)
            {
                remoteIt->isResolved = true;
                remoteIt->ips.clear();

                const auto &ips = result->ips();
                for (const auto &ip : ips)
                {
                    IpItem ipItem;
                    ipItem.ip = QString::fromStdString(ip);
                    ipItem.pingTime = pingManager_.getPing(ipItem.ip);
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
                    allIps << PingIpInfo { ipIt->ip, QString(), it->customConfig->name(), it->customConfig->nick(), wsnet::PingType::kIcmp };
                }
            }
            else
            {
                strListIps << remoteIt->ipOrHostname.ip;
                allIps << PingIpInfo { remoteIt->ipOrHostname.ip, QString(), it->customConfig->name(), it->customConfig->nick(), wsnet::PingType::kIcmp };
            }
        }
    }
    emit whitelistIpsChanged(strListIps);
    pingManager_.updateIps(allIps);
}

void CustomConfigLocationsModel::generateLocationsUpdated()
{
    QSharedPointer<types::Location> item(new types::Location());

    item->id = LocationID::createTopCustomConfigsLocationId();
    item->name = QObject::tr("Custom Configs");
    item->countryCode = "noflag";
    item->isPremiumOnly = false;
    item->isNoP2P = false;

    if (!pingInfos_.isEmpty())
    {
        for (const CustomConfigWithPingInfo &config : pingInfos_)
        {
            types::City city;
            city.id = LocationID::createCustomConfigLocationId(config.customConfig->filename());
            city.city = config.customConfig->name();
            city.nick = config.customConfig->nick();
            city.pingTimeMs = config.getPing();
            city.isPro = false;
            city.isDisabled = false;

            city.customConfigType = config.customConfig->type();
            city.customConfigIsCorrect = config.customConfig->isCorrect();
            city.customConfigErrorMessage = config.customConfig->getErrorForIncorrect();
            item->cities << city;
        }
    }

    emit locationsUpdated(item);
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
