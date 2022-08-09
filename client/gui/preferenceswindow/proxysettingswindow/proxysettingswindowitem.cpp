#include "proxysettingswindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

ProxySettingsWindowItem::ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences, &Preferences::proxySettingsChanged, this, &ProxySettingsWindowItem::onProxySettingsPreferencesChanged);

    proxySettingsItem_ = new ProxySettingsItem(this);
    proxySettingsItem_->setProxySettings(preferences->proxySettings());
    connect(proxySettingsItem_, &ProxySettingsItem::proxySettingsChanged, this,  &ProxySettingsWindowItem::onProxySettingsChanged);
    addItem(proxySettingsItem_);
}

QString ProxySettingsWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Proxy Settings");
}

void ProxySettingsWindowItem::onProxySettingsPreferencesChanged(const types::ProxySettings &ps)
{
    proxySettingsItem_->setProxySettings(ps);
}

void ProxySettingsWindowItem::onProxySettingsChanged(const types::ProxySettings &ps)
{
    preferences_->setProxySettings(ps);
}


} // namespace PreferencesWindow
