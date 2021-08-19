#include "proxysettingswindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

ProxySettingsWindowItem::ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences, SIGNAL(proxySettingsChanged(ProtoTypes::ProxySettings)), SLOT(onProxySettingsPreferencesChanged(ProtoTypes::ProxySettings)));

    proxySettingsItem_ = new ProxySettingsItem(this);
    proxySettingsItem_->setProxySettings(preferences->proxySettings());
    connect(proxySettingsItem_, SIGNAL(proxySettingsChanged(ProtoTypes::ProxySettings)), SLOT(onProxySettingsChanged(ProtoTypes::ProxySettings)));
    addItem(proxySettingsItem_);
}

QString ProxySettingsWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Proxy Settings");
}

void ProxySettingsWindowItem::onProxySettingsPreferencesChanged(const ProtoTypes::ProxySettings &ps)
{
    proxySettingsItem_->setProxySettings(ps);
}

void ProxySettingsWindowItem::onProxySettingsChanged(const ProtoTypes::ProxySettings &ps)
{
    preferences_->setProxySettings(ps);
}


} // namespace PreferencesWindow
