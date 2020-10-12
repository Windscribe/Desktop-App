#include "customconfiglocationinfo.h"
#include "utils/ipvalidation.h"
#include "engine/dnsresolver/dnsresolver.h"

namespace locationsmodel {

CustomConfigLocationInfo::CustomConfigLocationInfo(const LocationID &locationId, QSharedPointer<const customconfigs::ICustomConfig> config) :
    BaseLocationInfo(locationId, config->filename()), config_(config)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void*)), SLOT(onResolved(QString,QHostInfo,void*)));
}

bool CustomConfigLocationInfo::isExistSelectedNode() const
{
    return true;
}

void CustomConfigLocationInfo::resolveHostnames()
{
    bool isExistsHostnames = false;
    const QStringList hostnames = config_->hostnames();
    for (auto hostname : hostnames)
    {
        if (IpValidation::instance().isIp(hostname))
        {
            RemoteDescr rd;
            rd.ipOrHostname_ = hostname;
            rd.isHostname = false;
            remotes_ << rd;
        }
        else
        {
            RemoteDescr rd;
            rd.ipOrHostname_ = hostname;
            rd.isHostname = true;
            rd.isResolved = false;
            remotes_ << rd;

            isExistsHostnames = true;
            DnsResolver::instance().lookup(hostname, this);
        }
    }
    if (!isExistsHostnames)
    {
        emit hostnamesResolved();
    }
}

QString CustomConfigLocationInfo::getSelectedIp() const
{
    return "";
}

QString CustomConfigLocationInfo::getOvpnData() const
{
    return "";
}

QString CustomConfigLocationInfo::getFilename() const
{
    return config_->filename();
}

void CustomConfigLocationInfo::onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    if (userPointer == this)
    {
        for (int i = 0; i < remotes_.count(); ++i)
        {
            if (remotes_[i].isHostname && remotes_[i].ipOrHostname_ == hostname)
            {
                for (const QHostAddress ha : hostInfo.addresses())
                {
                    remotes_[i].ipsForHostname_ << ha.toString();
                }
                remotes_[i].isResolved = true;
                break;
            }
        }

        if (isAllResolved())
        {
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
