#include "customconfiglocationinfo.h"
#include "utils/ws_assert.h"
#include "utils/ipvalidation.h"
#include "utils/log/categories.h"
#include "engine/customconfigs/ovpncustomconfig.h"
#include "engine/customconfigs/wireguardcustomconfig.h"

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
        RemoteDescr rd;
        rd.ipOrHostname_ = remote;
        rd.isHostname = false;
        rd.port = globalPort_;
        if (!IpValidation::isIpv4Address(remote))
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
            WSNet::instance()->dnsResolver()->lookup(remote.toStdString(), 0, callback);
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
        if (IpValidation::isIpv4Address(remote.hostname))
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
            WSNet::instance()->dnsResolver()->lookup(remote.hostname.toStdString(), 0, callback);
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
    for (int i = 0; i < remotes_.count(); ++i) {
        if (remotes_[i].isHostname && remotes_[i].ipOrHostname_ == QString::fromStdString(hostname)) {
            std::string strIps;
            auto ips = result->ips();
            for (const auto &ip : ips)
            {
                remotes_[i].ipsForHostname_ << QString::fromStdString(ip);
                strIps += ip + "; ";
            }

            qCInfo(LOG_CONNECTION) << "Hostname:" << QString::fromStdString(hostname) << " resolved -> " << QString::fromStdString(strIps);
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
