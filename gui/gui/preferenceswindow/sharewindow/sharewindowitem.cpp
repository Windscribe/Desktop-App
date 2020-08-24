#include "sharewindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

ShareWindowItem::ShareWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent),
    preferences_(preferences), isWifiSharingSupported_(preferencesHelper->isWifiSharingSupported())
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences_, SIGNAL(shareSecureHotspotChanged(ProtoTypes::ShareSecureHotspot)), SLOT(onSecureHotspotParsPreferencesChanged(ProtoTypes::ShareSecureHotspot)));
    connect(preferences_, SIGNAL(shareProxyGatewayChanged(ProtoTypes::ShareProxyGateway)), SLOT(onProxyGatewayParsPreferencesChanged(ProtoTypes::ShareProxyGateway)));
    connect(preferences_, SIGNAL(connectionSettingsChanged(ProtoTypes::ConnectionSettings)), SLOT(onConnectionSettingsPreferencesChanged(ProtoTypes::ConnectionSettings)));
    connect(preferencesHelper, SIGNAL(wifiSharingSupportedChanged(bool)), SLOT(onPreferencesHelperWifiSharingSupportedChanged(bool)));

    secureHotspotItem_ = new SecureHotspotItem(this);
    connect(secureHotspotItem_, SIGNAL(secureHotspotParsChanged(ProtoTypes::ShareSecureHotspot)), SLOT(onSecureHotspotParsChanged(ProtoTypes::ShareSecureHotspot)));
    updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(preferences_->getEngineSettings().connection_settings()));
    secureHotspotItem_->setSecureHotspotPars(preferences->shareSecureHotspot());
    addItem(secureHotspotItem_);

    proxyGatewayItem_ = new ProxyGatewayItem(this, preferencesHelper);
    connect(proxyGatewayItem_, SIGNAL(proxyGatewayParsChanged(ProtoTypes::ShareProxyGateway)), SLOT(onProxyGatewayParsChanged(ProtoTypes::ShareProxyGateway)));
    connect(proxyGatewayItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(proxyGatewayItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));
    proxyGatewayItem_->setProxyGatewayPars(preferences->shareProxyGateway());
    addItem(proxyGatewayItem_);
}

QString ShareWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Share");
}

void ShareWindowItem::onSecureHotspotParsChanged(const ProtoTypes::ShareSecureHotspot &ss)
{
    preferences_->setShareSecureHotspot(ss);
}

void ShareWindowItem::onSecureHotspotParsPreferencesChanged(const ProtoTypes::ShareSecureHotspot &ss)
{
    secureHotspotItem_->setSecureHotspotPars(ss);
}

void ShareWindowItem::onConnectionSettingsPreferencesChanged(const ProtoTypes::ConnectionSettings &cs)
{
    updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(cs));
}

void ShareWindowItem::onProxyGatewayParsChanged(const ProtoTypes::ShareProxyGateway &sp)
{
    preferences_->setShareProxyGateway(sp);
}

void ShareWindowItem::onProxyGatewayParsPreferencesChanged(const ProtoTypes::ShareProxyGateway &sp)
{
    proxyGatewayItem_->setProxyGatewayPars(sp);
}

void ShareWindowItem::onPreferencesHelperWifiSharingSupportedChanged(bool bSupported)
{
    isWifiSharingSupported_ = bSupported;
    updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(preferences_->getEngineSettings().connection_settings()));
}

bool ShareWindowItem::isIkev2OrAutomaticConnectionMode(const ProtoTypes::ConnectionSettings &cs) const
{
    return cs.protocol() == ProtoTypes::PROTOCOL_IKEV2 || cs.is_automatic();
}

void ShareWindowItem::updateIsSupported(bool isWifiSharingSupported, bool isIkev2OrAutomatic)
{
    if (!isWifiSharingSupported)
    {
        secureHotspotItem_->setSupported(SecureHotspotItem::HOTSPOT_NOT_SUPPORTED);
    }
    else if (isIkev2OrAutomatic)
    {
        secureHotspotItem_->setSupported(SecureHotspotItem::HOTSPOT_NOT_SUPPORTED_BY_IKEV2);
    }
    else
    {
        secureHotspotItem_->setSupported(SecureHotspotItem::HOTSPOT_SUPPORTED);
    }
}

} // namespace PreferencesWindow
