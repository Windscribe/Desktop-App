#include "customconfiglocationsmodel.h"

#include <QFile>
#include <QTextStream>

#include "customconfiglocationinfo.h"
#include "engine/customconfigs/ovpncustomconfig.h"
#include "engine/customconfigs/wireguardcustomconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/ws_assert.h"

namespace locationsmodel {

using namespace wsnet;

CustomConfigLocationsModel::CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    pingManager_(this, stateController, networkDetectionManager, "pingStorageCustomConfigs")
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
            // IPv6 endpoint connections are not implemented yet — skip literal v6 endpoints so
            // we don't ping/whitelist addresses we'll never connect to. Hostnames that *resolve*
            // to v6-only are handled in onDnsRequestFinished.
            if (NetworkingValidation::isIpv6(hostname))
            {
                qCWarning(LOG_BASIC) << "Custom config endpoint" << hostname
                                     << "is an IPv6 literal, skipping. IPv6 endpoint connections are not implemented yet.";
                continue;
            }

            RemoteItem ri;
            ri.ipOrHostname.ip = hostname;
            ri.isHostname = !NetworkingValidation::isIp(hostname);

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
        QMetaObject::invokeMethod(this, [this, hostname, result] { // NOLINT: false positive for memory leak
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
            // Custom configs may legitimately point at v6-only or dual-stack hostnames. Ask for
            // both families so we can surface a clear warning when only IPv6 is returned; the
            // VPN data path is still IPv4-only — see onDnsRequestFinished().
            WSNet::instance()->dnsResolver()->lookup(hostname.toStdString(), 0, IpFamily::kBoth, callback);
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
    // We request kBoth (see setCustomConfigs) so v6-only hostnames don't silently look like a
    // DNS failure. The VPN data path is still IPv4-only today, so keep only v4 results here and
    // log a clear warning when only IPv6 came back — that's the actionable signal for users
    // pointing custom configs at v6-only endpoints.
    QStringList ipv4Strings;
    QStringList ipv6Strings;
    for (const auto &ip : result->ips()) {
        const QString qip = QString::fromStdString(ip);
        if (NetworkingValidation::isIpv4(qip)) {
            ipv4Strings << qip;
        } else if (NetworkingValidation::isIpv6(qip)) {
            ipv6Strings << qip;
        }
    }

    if (ipv4Strings.isEmpty() && !ipv6Strings.isEmpty()) {
        qCWarning(LOG_BASIC) << "Custom config hostname:" << hostname
                             << "resolved only to IPv6 addresses, skipping. IPv6 endpoint connections are not implemented yet. Addresses:"
                             << ipv6Strings.join("; ");
    } else if (!ipv6Strings.isEmpty()) {
        qCInfo(LOG_BASIC) << "Custom config hostname:" << hostname
                          << "also has IPv6 addresses, ignored (IPv6 endpoints are not supported yet):"
                          << ipv6Strings.join("; ");
    }

    for (auto it = pingInfos_.begin(); it != pingInfos_.end(); ++it)
    {
        for (auto remoteIt = it->remotes.begin(); remoteIt != it->remotes.end(); ++remoteIt)
        {
            if (remoteIt->isHostname && remoteIt->ipOrHostname.ip == hostname)
            {
                remoteIt->isResolved = true;
                remoteIt->ips.clear();

                for (const QString &ip : ipv4Strings)
                {
                    IpItem ipItem;
                    ipItem.ip = ip;
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
    item->name = QObject::tr("Custom configs");
    item->countryCode = "noflag";
    item->isPremiumOnly = false;

    if (!pingInfos_.isEmpty())
    {
        for (const CustomConfigWithPingInfo &config : pingInfos_)
        {
            types::City city;
            city.id = LocationID::createCustomConfigLocationId(config.customConfig->filename());
            city.city = config.customConfig->name();
            city.nick = config.customConfig->nick();
            city.pingTimeMs = config.getPing();
            city.isPremiumOnly = false;
            city.isDisabled = false;

            city.customConfigType = config.customConfig->type();
            if (config.customConfig->type() == CUSTOM_CONFIG_OPENVPN) {
                const auto *ovpn = static_cast<const customconfigs::OvpnCustomConfig *>(config.customConfig.get());
                city.customConfigProtocol = (ovpn->globalProtocol() == "tcp") ? types::Protocol::OPENVPN_TCP : types::Protocol::OPENVPN_UDP;
                city.customConfigPort = ovpn->globalPort();
                if (city.customConfigPort == 0 && !ovpn->remotes().isEmpty()) {
                    city.customConfigPort = ovpn->remotes().first().port;
                }
            } else {
                const auto *wg = static_cast<const customconfigs::WireguardCustomConfig *>(config.customConfig.get());
                city.customConfigProtocol = types::Protocol::WIREGUARD;
                city.customConfigPort = wg->getEndpointPort();
            }
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
