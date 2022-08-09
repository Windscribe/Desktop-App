#include "sharewindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

ShareWindowItem::ShareWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent),
    preferences_(preferences), isWifiSharingSupported_(preferencesHelper->isWifiSharingSupported())
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences_, &Preferences::shareSecureHotspotChanged, this, &ShareWindowItem::onSecureHotspotParsPreferencesChanged);
    connect(preferences_, &Preferences::shareProxyGatewayChanged, this, &ShareWindowItem::onProxyGatewayParsPreferencesChanged);
    connect(preferences_, &Preferences::connectionSettingsChanged, this, &ShareWindowItem::onConnectionSettingsPreferencesChanged);
    connect(preferencesHelper, &PreferencesHelper::wifiSharingSupportedChanged, this, &ShareWindowItem::onPreferencesHelperWifiSharingSupportedChanged);

#ifndef Q_OS_LINUX
    secureHotspotItem_ = new SecureHotspotItem(this);
    connect(secureHotspotItem_, &SecureHotspotItem::secureHotspotParsChanged, this, &ShareWindowItem::onSecureHotspotParsChanged);
    //updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(preferences_->getEngineSettings().connection_settings()));
    secureHotspotItem_->setSecureHotspotPars(preferences->shareSecureHotspot());
    addItem(secureHotspotItem_);
#endif

    proxyGatewayItem_ = new ProxyGatewayItem(this, preferencesHelper);
    connect(proxyGatewayItem_, &ProxyGatewayItem::proxyGatewayParsChanged, this, &ShareWindowItem::onProxyGatewayParsChanged);
    proxyGatewayItem_->setProxyGatewayPars(preferences->shareProxyGateway());
    addItem(proxyGatewayItem_);
}

QString ShareWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Share");
}

void ShareWindowItem::onSecureHotspotParsChanged(const types::ShareSecureHotspot &ss)
{
    preferences_->setShareSecureHotspot(ss);
}

void ShareWindowItem::onSecureHotspotParsPreferencesChanged(const types::ShareSecureHotspot &ss)
{
    if (secureHotspotItem_) {
        secureHotspotItem_->setSecureHotspotPars(ss);
    }
}

void ShareWindowItem::onConnectionSettingsPreferencesChanged(const types::ConnectionSettings &cs)
{
    updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(cs));
}

void ShareWindowItem::onProxyGatewayParsChanged(const types::ShareProxyGateway &sp)
{
    preferences_->setShareProxyGateway(sp);
}

void ShareWindowItem::onProxyGatewayParsPreferencesChanged(const types::ShareProxyGateway &sp)
{
    proxyGatewayItem_->setProxyGatewayPars(sp);
}

void ShareWindowItem::onPreferencesHelperWifiSharingSupportedChanged(bool bSupported)
{
    isWifiSharingSupported_ = bSupported;
    updateIsSupported(isWifiSharingSupported_, isIkev2OrAutomaticConnectionMode(preferences_->getEngineSettings().connectionSettings()));
}

bool ShareWindowItem::isIkev2OrAutomaticConnectionMode(const types::ConnectionSettings &cs) const
{
    return cs.protocol == PROTOCOL::IKEV2 || cs.protocol == PROTOCOL::WIREGUARD || cs.isAutomatic;
}

void ShareWindowItem::updateIsSupported(bool isWifiSharingSupported, bool isIkev2OrAutomatic)
{
    if (secureHotspotItem_) {
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
}

} // namespace PreferencesWindow
