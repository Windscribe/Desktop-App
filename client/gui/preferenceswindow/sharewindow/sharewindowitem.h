#ifndef SHAREWINDOWITEM_H
#define SHAREWINDOWITEM_H

#include "../basepage.h"
#include "securehotspotitem.h"
#include "proxygatewayitem.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"

namespace PreferencesWindow {

class ShareWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit ShareWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();

private slots:
    void onSecureHotspotParsChanged(const types::ShareSecureHotspot &ss);
    void onSecureHotspotParsPreferencesChanged(const types::ShareSecureHotspot &ss);

    void onConnectionSettingsPreferencesChanged(const types::ConnectionSettings &cs);

    void onProxyGatewayParsChanged(const types::ShareProxyGateway &sp);
    void onProxyGatewayParsPreferencesChanged(const types::ShareProxyGateway &sp);

    void onPreferencesHelperWifiSharingSupportedChanged(bool bSupported);

private:
    Preferences* const preferences_;
    bool isWifiSharingSupported_;
    SecureHotspotItem *secureHotspotItem_{ nullptr };
    ProxyGatewayItem *proxyGatewayItem_{ nullptr };

    bool isIkev2OrAutomaticConnectionMode(const types::ConnectionSettings &cs) const;
    void updateIsSupported(bool isWifiSharingSupported, bool isIkev2OrAutomatic);
};

} // namespace PreferencesWindow

#endif // SHAREWINDOWITEM_H
