#ifndef VPNSHARESETTINGS_H
#define VPNSHARESETTINGS_H

#include <QSettings>
#include "Engine/Types/types.h"

class VPNShareSettings
{
public:
    VPNShareSettings();

    //void set(const VPNShareSettings &vpnShareSettings);
    void readFromSettings(QSettings &settings);
    void writeToSettings();

    bool isProxyGatewayEnabled() const { return proxyGatewayEnabled_; }
    PROXY_SHARING_TYPE getProxyType() const { return proxyType_; }
    void setProxyGatewayEnabled(PROXY_SHARING_TYPE proxyType);
    void setProxyGatewayDisabled();
    void setProxyGatewayType(PROXY_SHARING_TYPE proxyType);

    bool isSecureHotspotEnabled() const { return secureHotspotEnabled_; }
    QString secureHotspotSsid() const { return ssid_; }
    QString secureHotspotPassword() const { return password_; }

    void setSecureHotspotEnabled(const QString &ssid, const QString &password);
    void setSecureHotspotDisabled();
    void setSecureHotspotSsidAndPassword(const QString &ssid, const QString &password);

    void debugToLog();
    QString getDebugString();

    friend bool operator!= (const VPNShareSettings &lhs, const VPNShareSettings &rhs);

private:
    bool proxyGatewayEnabled_;
    PROXY_SHARING_TYPE proxyType_;
    bool secureHotspotEnabled_;
    QString ssid_;
    QString password_;
};

bool operator!=(const VPNShareSettings& lhs, const VPNShareSettings& rhs);

#endif // VPNSHARESETTINGS_H
