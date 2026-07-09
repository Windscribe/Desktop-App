#include "customconfiglocationinfo.h"
#include "engine/customconfigs/ovpncustomconfig.h"
#include "engine/customconfigs/wireguardcustomconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/ws_assert.h"

using namespace wsnet;


namespace locationsmodel {

CustomConfigLocationInfo::CustomConfigLocationInfo(const LocationID &locationId, QSharedPointer<const customconfigs::ICustomConfig> config) :
    BaseLocationInfo(locationId, config->filename()), config_(config), globalPort_(0), bAllResolved_(false), selected_(0), selectedHostname_(0)
{
}

bool CustomConfigLocationInfo::isExistSelectedNode() const
{
    if (!config_->isCorrect())
    {
        return false;
    }

    if (bAllResolved_)
    {
        if (selected_ >= 0 && selected_ < remotes_.count())
        {
            if (!remotes_[selected_].isHostname)
            {
                return true;
            }
            else
            {
                return selectedHostname_ >= 0 &&
                       selectedHostname_ < remotes_[selected_].ipsForHostname_.count();
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
}

void CustomConfigLocationInfo::resolveHostnames()
{
    if (bAllResolved_)
    {
        emit hostnamesResolved();
        return;
    }

    switch (config_->type()) {
    case CUSTOM_CONFIG_OPENVPN:
        resolveHostnamesForOVPNConfig();
        break;
    case CUSTOM_CONFIG_WIREGUARD:
        resolveHostnamesForWireGuardConfig();
        break;
    default:
        WS_ASSERT(false);
        break;
    }
}

void CustomConfigLocationInfo::resolveHostnamesForWireGuardConfig()
{
    auto *config = dynamic_cast<customconfigs::WireguardCustomConfig const *>(config_.data());
    if (!config)
    {
        WS_ASSERT(false);
        return;
    }

    globalPort_ = config->getEndpointPort();
    globalProtocol_ = "WireGuard";

    bool isExistsHostnames = false;
    const auto remotes = config->hostnames();
    for (const auto &remote : remotes)
    {
        // IPv6 endpoint connections are not implemented yet (see also onDnsRequestFinished).
        // Skip literal v6 endpoints with a clear warning so they don't silently break the
        // firewall path.
        if (NetworkingValidation::isIpv6(remote))
        {
            qCWarning(LOG_CONNECTION) << "Custom WireGuard config endpoint" << remote
                                      << "is an IPv6 literal, skipping. IPv6 endpoint connections are not implemented yet.";
            continue;
        }
        RemoteDescr rd;
        rd.ipOrHostname_ = remote;
        rd.isHostname = false;
        rd.port = globalPort_;
        if (!NetworkingValidation::isIp(remote))
        {
            rd.isHostname = true;
            rd.isResolved = false;
            isExistsHostnames = true;

            auto callback = [this] (std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
            {
                QMetaObject::invokeMethod(this, [this, hostname, result] { // NOLINT: false positive for memory leak
                    onDnsRequestFinished(hostname, result);
                });
            };
            // Custom configs may legitimately point at v6-only or dual-stack hostnames. We must
            // ask for both families so we can see when the resolver returns only IPv6 and warn
            // the user clearly instead of silently failing the firewall path. The actual
            // connection still uses IPv4 only — see onDnsRequestFinished().
            WSNet::instance()->dnsResolver()->lookup(remote.toStdString(), 0, IpFamily::kBoth, callback);
        }
        remotes_ << rd;
    }
    if (!isExistsHostnames)
    {
        bAllResolved_ = true;
        emit hostnamesResolved();
    }
}

void CustomConfigLocationInfo::resolveHostnamesForOVPNConfig()
{
    auto *config = dynamic_cast<customconfigs::OvpnCustomConfig const *>(config_.data());
    if (!config)
    {
        WS_ASSERT(false);
        return;
    }

    globalPort_ = config->globalPort();
    globalProtocol_ = config->globalProtocol();

    bool isExistsHostnames = false;
    const QVector<customconfigs::RemoteCommandLine> remotes = config->remotes();
    for (const auto &remote : remotes)
    {
        // IPv6 endpoint connections are not implemented yet (see also onDnsRequestFinished).
        if (NetworkingValidation::isIpv6(remote.hostname))
        {
            qCWarning(LOG_CONNECTION) << "Custom OpenVPN config remote" << remote.hostname
                                      << "is an IPv6 literal, skipping. IPv6 endpoint connections are not implemented yet.";
            continue;
        }
        if (NetworkingValidation::isIp(remote.hostname))
        {
            RemoteDescr rd;
            rd.ipOrHostname_ = remote.hostname;
            rd.isHostname = false;

            rd.port = remote.port;
            rd.protocol = remote.protocol;
            rd.remoteCmdLine = remote.originalRemoteCommand;

            remotes_ << rd;
        }
        else
        {
            RemoteDescr rd;
            rd.ipOrHostname_ = remote.hostname;
            rd.isHostname = true;
            rd.isResolved = false;

            rd.port = remote.port;
            rd.protocol = remote.protocol;
            rd.remoteCmdLine = remote.originalRemoteCommand;

            remotes_ << rd;

            isExistsHostnames = true;

            auto callback = [this] (std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
            {
                QMetaObject::invokeMethod(this, [this, hostname, result] { // NOLINT: false positive for memory leak
                    onDnsRequestFinished(hostname, result);
                });
            };
            WSNet::instance()->dnsResolver()->lookup(remote.hostname.toStdString(), 0, IpFamily::kBoth, callback);
        }
    }
    if (!isExistsHostnames)
    {
        bAllResolved_ = true;
        emit hostnamesResolved();
    }
}

QString CustomConfigLocationInfo::getSelectedIp() const
{
    WS_ASSERT(selected_ >= 0 && selected_ < remotes_.count());

    if (!remotes_[selected_].isHostname)
    {
        return remotes_[selected_].ipOrHostname_;
    }
    else
    {
        return remotes_[selected_].ipsForHostname_[selectedHostname_];
    }
}

QString CustomConfigLocationInfo::getSelectedRemoteCommand() const
{
    WS_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
    return remotes_[selected_].remoteCmdLine;
}

uint CustomConfigLocationInfo::getSelectedPort() const
{
    WS_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
    if (remotes_[selected_].port != 0)
    {
        return remotes_[selected_].port;
    }
    else
    {
        return globalPort_;
    }
}

QString CustomConfigLocationInfo::getSelectedProtocol() const
{
    WS_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
    if (!remotes_[selected_].protocol.isEmpty())
    {
        return remotes_[selected_].protocol;
    }
    else
    {
        return globalProtocol_;
    }
}

QString CustomConfigLocationInfo::getOvpnData() const
{
    if (config_->type() == CUSTOM_CONFIG_OPENVPN) {
        auto *config = dynamic_cast<customconfigs::OvpnCustomConfig const *>(config_.data());
        if (config)
            return config->getOvpnData();
        WS_ASSERT(false);
    }
    return QString();
}

QString CustomConfigLocationInfo::getFilename() const
{
    return config_->filename();
}

QSharedPointer<WireGuardConfig> CustomConfigLocationInfo::getWireguardCustomConfig(
    const QString &endpointIp) const
{
    if (config_->type() == CUSTOM_CONFIG_WIREGUARD) {
        auto *config = dynamic_cast<customconfigs::WireguardCustomConfig const *>(config_.data());
        if ( config )
            return config->getWireGuardConfig(endpointIp);
        WS_ASSERT(false);
    }
    return nullptr;
}

bool CustomConfigLocationInfo::isAllowFirewallAfterConnection() const
{
    return config_->isAllowFirewallAfterConnection();
}

void CustomConfigLocationInfo::selectNextNode()
{
    if (remotes_[selected_].isHostname)
    {
        if (selectedHostname_ < (remotes_[selected_].ipsForHostname_.count() - 1))
        {
            selectedHostname_++;
        }
        else
        {
            if (selected_ < (remotes_.count() - 1))
            {
                selected_++;
            }
            else
            {
                selected_ = 0;
            }
            selectedHostname_ = 0;
        }
    }
    else
    {
        if (selected_ < (remotes_.count() - 1))
        {
            selected_++;
        }
        else
        {
            selected_ = 0;
        }
    }
}

QString CustomConfigLocationInfo::getLogString() const
{
    QString ret = "Remotes: ";
    QStringList hostnames = config_->hostnames();
    for (int i = 0; i < hostnames.count(); ++i)
    {
        ret += hostnames[i];
        if (i < (hostnames.count() - 1))
        {
            ret += "; ";
        }
    }
    return ret;
}

void CustomConfigLocationInfo::onDnsRequestFinished(const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
{
    const QString qHostname = QString::fromStdString(hostname);
    for (int i = 0; i < remotes_.count(); ++i) {
        if (remotes_[i].isHostname && remotes_[i].ipOrHostname_ == qHostname) {
            // We requested kBoth so we can detect v6-only / dual-stack cases explicitly. The
            // VPN data path is still v4-only today, so we filter to v4 here. When v6 was the
            // only answer, log a clear warning and leave ipsForHostname_ empty — that drives
            // isExistSelectedNode()=false on this remote so doConnect() reports a real error
            // instead of silently being killed by the firewall.
            QStringList ipv4List;
            QStringList ipv6List;
            for (const auto &ip : result->ips()) {
                const QString qip = QString::fromStdString(ip);
                if (NetworkingValidation::isIpv4(qip)) {
                    ipv4List << qip;
                } else if (NetworkingValidation::isIpv6(qip)) {
                    ipv6List << qip;
                }
            }
            remotes_[i].ipsForHostname_ = ipv4List;

            if (!ipv4List.isEmpty()) {
                qCInfo(LOG_CONNECTION) << "Hostname:" << qHostname << "resolved (IPv4) ->" << ipv4List.join("; ");
                if (!ipv6List.isEmpty()) {
                    qCInfo(LOG_CONNECTION) << "Hostname:" << qHostname
                                           << "also has IPv6 addresses, ignored (IPv6 endpoints are not supported yet):"
                                           << ipv6List.join("; ");
                }
            } else if (!ipv6List.isEmpty()) {
                qCWarning(LOG_CONNECTION) << "Hostname:" << qHostname
                                          << "resolved only to IPv6 addresses, skipping. IPv6 endpoint connections are not implemented yet. Addresses:"
                                          << ipv6List.join("; ");
            } else {
                qCWarning(LOG_CONNECTION) << "Hostname:" << qHostname << "did not resolve to any IP";
            }
            remotes_[i].isResolved = true;
            break;
        }
    }

    if (isAllResolved()) {
        bAllResolved_ = true;
        emit hostnamesResolved();
    }
}

bool CustomConfigLocationInfo::isAllResolved() const
{
    for (int i = 0; i < remotes_.count(); ++i)
    {
        if (remotes_[i].isHostname && !remotes_[i].isResolved)
        {
            return false;
        }
    }
    return true;
}


} //namespace locationsmodel
