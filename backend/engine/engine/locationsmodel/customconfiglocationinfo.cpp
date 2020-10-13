#include "customconfiglocationinfo.h"
#include "utils/ipvalidation.h"
#include "utils/logger.h"
#include "engine/dnsresolver/dnsresolver.h"
#include "engine/customconfigs/ovpncustomconfig.h"

namespace locationsmodel {

CustomConfigLocationInfo::CustomConfigLocationInfo(const LocationID &locationId, QSharedPointer<const customconfigs::ICustomConfig> config) :
    BaseLocationInfo(locationId, config->filename()), config_(config), globalPort_(0), bAllResolved_(false), selected_(0), selectedHostname_(0)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void*)), SLOT(onResolved(QString,QHostInfo,void*)));
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
                return selectedHostname_ >= 0 && selectedHostname_ < remotes_[selected_].ipsForHostname_.count();
            }
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

    customconfigs::OvpnCustomConfig const *config = dynamic_cast<customconfigs::OvpnCustomConfig const *>(config_.data());
    if (!config)
    {
        Q_ASSERT(false);
        return;
    }

    globalPort_ = config->globalPort();
    globalProtocol_ = config->globalProtocol();

    bool isExistsHostnames = false;
    const QVector<customconfigs::RemoteCommandLine> remotes = config->remotes();
    for (const auto &remote : remotes)
    {
        if (IpValidation::instance().isIp(remote.hostname))
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
            DnsResolver::instance().lookup(remote.hostname, this);
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
    Q_ASSERT(selected_ >= 0 && selected_ < remotes_.count());

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
    Q_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
    return remotes_[selected_].remoteCmdLine;
}

uint CustomConfigLocationInfo::getSelectedPort() const
{
    Q_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
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
    Q_ASSERT(selected_ >= 0 && selected_ < remotes_.count());
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
    customconfigs::OvpnCustomConfig const *config = dynamic_cast<customconfigs::OvpnCustomConfig const *>(config_.data());
    if (config)
    {
        return config->getOvpnData();
    }
    else
    {
        Q_ASSERT(false);
        return QString();
    }
}

QString CustomConfigLocationInfo::getFilename() const
{
    return config_->filename();
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

void CustomConfigLocationInfo::onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    if (userPointer == this)
    {
        for (int i = 0; i < remotes_.count(); ++i)
        {
            if (remotes_[i].isHostname && remotes_[i].ipOrHostname_ == hostname)
            {
                QString strIps;
                for (const QHostAddress ha : hostInfo.addresses())
                {
                    remotes_[i].ipsForHostname_ << ha.toString();
                    strIps += ha.toString() + "; ";
                }

                qCDebug(LOG_CONNECTION) << "Hostname:" << hostname << " resolved -> " << strIps;
                remotes_[i].isResolved = true;
                break;
            }
        }

        if (isAllResolved())
        {
            bAllResolved_ = true;
            emit hostnamesResolved();
        }
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
