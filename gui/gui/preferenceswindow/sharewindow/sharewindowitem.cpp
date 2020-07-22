#include "sharewindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

ShareWindowItem::ShareWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences_, SIGNAL(shareSecureHotspotChanged(ProtoTypes::ShareSecureHotspot)), SLOT(onSecureHotspotParsPreferencesChanged(ProtoTypes::ShareSecureHotspot)));
    connect(preferences_, SIGNAL(shareProxyGatewayChanged(ProtoTypes::ShareProxyGateway)), SLOT(onProxyGatewayParsPreferencesChanged(ProtoTypes::ShareProxyGateway)));
    connect(preferencesHelper, SIGNAL(wifiSharingSupportedChanged(bool)), SLOT(onPreferencesHelperWifiSharingSupportedChanged(bool)));

    secureHotspotItem_ = new SecureHotspotItem(this);
    connect(secureHotspotItem_, SIGNAL(secureHotspotParsChanged(ProtoTypes::ShareSecureHotspot)), SLOT(onSecureHotspotParsChanged(ProtoTypes::ShareSecureHotspot)));
    secureHotspotItem_->setSupported(preferencesHelper->isWifiSharingSupported());
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
    secureHotspotItem_->setSupported(bSupported);
}

} // namespace PreferencesWindow
