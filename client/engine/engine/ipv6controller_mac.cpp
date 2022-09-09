#include "ipv6controller_mac.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

void Ipv6Controller_mac::setHelper(IHelper *helper)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);
}

void Ipv6Controller_mac::disableIpv6()
{
    if (!bIsDisabled_)
    {
        WS_ASSERT(helper_ != NULL);
        qCDebug(LOG_BASIC) << "Disable IPv6 for all network interfaces";

        const QString answer = helper_->executeRootCommand("networksetup -listallnetworkservices");
        const QStringList strs = answer.split("\n");
        QStringList networkIntefaces;
        for (const QString &str : strs)
        {
            if (!str.isEmpty() && !str.contains("An asterisk", Qt::CaseInsensitive))
            {
                networkIntefaces << str;
            }
        }

        saveIpv6States(networkIntefaces);

        for (const QString &networkInterface : qAsConst(networkIntefaces))
        {
            if (isEnabledIPv6(networkInterface))
            {
                QString cmd = "networksetup -setv6off \"" + networkInterface + "\"";
                qCDebug(LOG_BASIC) << "Execute cmd:" << cmd;
                helper_->executeRootCommand(cmd);
            }
        }

        bIsDisabled_ = true;
    }
}

void Ipv6Controller_mac::restoreIpv6()
{
    if (bIsDisabled_)
    {
        WS_ASSERT(helper_ != NULL);
        qCDebug(LOG_BASIC) << "Restore IPv6 for all network interfaces";

        const QString answer = helper_->executeRootCommand("networksetup -listallnetworkservices");
        const QStringList strs = answer.split("\n");
        QStringList networkIntefaces;
        for (const QString &str : strs)
        {
            if (!str.isEmpty() && !str.contains("An asterisk", Qt::CaseInsensitive))
            {
                networkIntefaces << str;
            }
        }

        for(const QString &networkInterface : qAsConst(networkIntefaces))
        {
            QString curState = getStateFromCmd(networkInterface);
            QString savedState = getStateFromSaved(networkInterface);

            bool bNeedSetToAutomatic = false;

            if (curState != savedState && savedState == "Automatic")
            {
                bNeedSetToAutomatic = true;
            }
            else if (curState.isEmpty() || savedState.isEmpty())
            {
                bNeedSetToAutomatic = true;
            }
            else if (curState == "Unknown" || savedState == "Unknown")
            {
                bNeedSetToAutomatic = true;
            }

            if (bNeedSetToAutomatic)
            {
                QString cmd = "networksetup -setv6automatic \"" + networkInterface + "\"";
                qCDebug(LOG_BASIC) << "Execute cmd:" << cmd;
                helper_->executeRootCommand(cmd);
            }
        }

        bIsDisabled_ = false;
    }
}

Ipv6Controller_mac::Ipv6Controller_mac() : bIsDisabled_(false), helper_(NULL)
{

}

Ipv6Controller_mac::~Ipv6Controller_mac()
{
    WS_ASSERT(bIsDisabled_ == false);
}

void Ipv6Controller_mac::saveIpv6States(const QStringList &networkIntefaces)
{
    for (const QString &interface : networkIntefaces)
    {
        const QString answer = helper_->executeRootCommand("networksetup -getinfo \"" + interface + "\"");
        qCDebug(LOG_BASIC) << "Saved config for interface" << interface << ":" << answer;
        const QStringList strs = answer.split("\n");
        for (const QString &s : strs)
        {
            if (s.contains("IPv6:", Qt::CaseInsensitive))
            {
                if (s.contains("Automatic", Qt::CaseInsensitive))
                {
                    ipv6States_[interface] = "Automatic";
                }
                else if (s.contains("Off", Qt::CaseInsensitive))
                {
                    ipv6States_[interface] = "Off";
                }
                else
                {
                    qCDebug(LOG_BASIC) << "Unknown state for interface" << interface << ":" << s;
                    ipv6States_[interface] = "Unknown";
                }
                break;
            }
        }
    }
}

bool Ipv6Controller_mac::isEnabledIPv6(const QString &interface)
{
    auto it = ipv6States_.find(interface);
    if (it != ipv6States_.end())
    {
        if (it.value() == "Off")
        {
            return false;
        }
    }
    return true;
}

QString Ipv6Controller_mac::getStateFromSaved(const QString &interface)
{
    auto it = ipv6States_.find(interface);
    if (it != ipv6States_.end())
    {
        return it.value();
    }
    return "";
}

QString Ipv6Controller_mac::getStateFromCmd(const QString &interface)
{
    const QString answer = helper_->executeRootCommand("networksetup -getinfo \"" + interface + "\"");
    const QStringList strs = answer.split("\n");
    for (const QString &s : strs)
    {
        if (s.contains("IPv6:", Qt::CaseInsensitive))
        {
            if (s.contains("Automatic", Qt::CaseInsensitive))
            {
                return "Automatic";
            }
            else if (s.contains("Off", Qt::CaseInsensitive))
            {
                return "Off";
            }
            else
            {
                qCDebug(LOG_BASIC) << "Unknown state for interface" << interface << ":" << s;
                return "Unknown";
            }
        }
    }
    return "";
}
