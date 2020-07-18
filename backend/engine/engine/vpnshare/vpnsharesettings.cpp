#include "vpnsharesettings.h"
#include "Utils/logger.h"

bool operator!=(const VPNShareSettings& lhs, const VPNShareSettings& rhs)
{
    return lhs.proxyGatewayEnabled_ != rhs.proxyGatewayEnabled_ || lhs.proxyType_ != rhs.proxyType_ ||
           lhs.secureHotspotEnabled_ != rhs.secureHotspotEnabled_ || lhs.ssid_ != rhs.ssid_ || lhs.password_ != rhs.password_;
}

VPNShareSettings::VPNShareSettings() : proxyGatewayEnabled_(false), proxyType_(PROXY_SHARING_HTTP),
    secureHotspotEnabled_(false)
{

}

/*void VPNShareSettings::set(const VPNShareSettings &vpnShareSettings)
{
    proxyGatewayEnabled_ = vpnShareSettings.proxyGatewayEnabled_;
    proxyType_ = vpnShareSettings.proxyType_;
    secureHotspotEnabled_ = vpnShareSettings.secureHotspotEnabled_;
    ssid_ = vpnShareSettings.ssid_;
    password_ = vpnShareSettings.password_;
}*/

void VPNShareSettings::readFromSettings(QSettings &settings)
{
    proxyGatewayEnabled_ = settings.value("proxyGatewayEnabled", false).toBool();
    proxyType_ = (PROXY_SHARING_TYPE)settings.value("proxyGatewayType", PROXY_SHARING_HTTP).toInt();
    secureHotspotEnabled_ = settings.value("secureHotspotEnabled", false).toBool();
    ssid_ = settings.value("secureHotspotSsid", "").toString();
    password_ = settings.value("secureHotspotPassword", "").toString();
}

void VPNShareSettings::writeToSettings()
{
    QSettings settings;
    settings.setValue("proxyGatewayEnabled", proxyGatewayEnabled_);
    settings.setValue("proxyGatewayType", (int)proxyType_);
    settings.setValue("secureHotspotEnabled", secureHotspotEnabled_);
    settings.setValue("secureHotspotSsid", ssid_);
    settings.setValue("secureHotspotPassword", password_);
}

void VPNShareSettings::setProxyGatewayEnabled(PROXY_SHARING_TYPE proxyType)
{
    proxyGatewayEnabled_ = true;
    proxyType_ = proxyType;
    writeToSettings();
}

void VPNShareSettings::setProxyGatewayDisabled()
{
    proxyGatewayEnabled_ = false;
    writeToSettings();
}

void VPNShareSettings::setProxyGatewayType(PROXY_SHARING_TYPE proxyType)
{
    proxyType_ = proxyType;
    writeToSettings();
}

void VPNShareSettings::setSecureHotspotEnabled(const QString &ssid, const QString &password)
{
    ssid_ = ssid;
    password_ = password;
    secureHotspotEnabled_ = true;
    writeToSettings();
}

void VPNShareSettings::setSecureHotspotDisabled()
{
    secureHotspotEnabled_ = false;
    writeToSettings();
}

void VPNShareSettings::setSecureHotspotSsidAndPassword(const QString &ssid, const QString &password)
{
    ssid_ = ssid;
    password_ = password;
    writeToSettings();
}

void VPNShareSettings::debugToLog()
{
    qCDebug(LOG_BASIC) << "Proxy sharing enabled:" << proxyGatewayEnabled_;
    if (proxyGatewayEnabled_)
    {
        if (proxyType_ == PROXY_SHARING_HTTP)
        {
            qCDebug(LOG_BASIC) << "Proxy type: HTTP";
        }
        else if (proxyType_ == PROXY_SHARING_SOCKS)
        {
            qCDebug(LOG_BASIC) << "Proxy type: SOCKS";
        }
        else
        {
            Q_ASSERT(false);
        }
    }

    qCDebug(LOG_BASIC) << "Secure hotspot enabled:" << secureHotspotEnabled_;
    if (secureHotspotEnabled_)
    {
        qCDebug(LOG_BASIC) << "SSID:" << ssid_;
    }
}

QString VPNShareSettings::getDebugString()
{
    QString ret;

    if (secureHotspotEnabled_)
    {
        ret += "Secure hotspot enabled; ";
    }
    else
    {
        ret += "Secure hotspot disabled; ";
    }

    if (proxyGatewayEnabled_)
    {
        if (proxyType_ == PROXY_SHARING_HTTP)
        {
            ret += "proxy gateway enabled = HTTP";
        }
        else if (proxyType_ == PROXY_SHARING_SOCKS)
        {
            ret += "proxy gateway enabled = SOCKS";
        }
        else
        {
            Q_ASSERT(false);
        }
    }
    else
    {
        ret += "proxy gateway disabled";
    }
    return ret;
}
