#ifndef SHAREWINDOWITEM_H
#define SHAREWINDOWITEM_H

#include "../basepage.h"
#include "securehotspotitem.h"
#include "proxygatewayitem.h"
#include "../Backend/Preferences/preferenceshelper.h"
#include "../Backend/Preferences/preferences.h"

namespace PreferencesWindow {

class ShareWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit ShareWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();

signals:
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onSecureHotspotParsChanged(const ProtoTypes::ShareSecureHotspot &ss);
    void onSecureHotspotParsPreferencesChanged(const ProtoTypes::ShareSecureHotspot &ss);

    void onProxyGatewayParsChanged(const ProtoTypes::ShareProxyGateway &sp);
    void onProxyGatewayParsPreferencesChanged(const ProtoTypes::ShareProxyGateway &sp);

    void onPreferencesHelperWifiSharingSupportedChanged(bool bSupported);

private:
    SecureHotspotItem *secureHotspotItem_;
    ProxyGatewayItem *proxyGatewayItem_;
    Preferences *preferences_;

};

} // namespace PreferencesWindow

#endif // SHAREWINDOWITEM_H
