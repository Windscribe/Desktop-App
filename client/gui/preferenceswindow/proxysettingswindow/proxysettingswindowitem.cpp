#include "proxysettingswindowitem.h"

#include <QPainter>
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

ProxySettingsWindowItem::ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : CommonGraphics::BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    connect(preferences, &Preferences::proxySettingsChanged, this, &ProxySettingsWindowItem::onProxySettingsPreferencesChanged);

    desc_ = new PreferenceGroup(this,
                                tr("If your network has a LAN proxy, configure it here."),
                                QString("https://%1/features/proxy-settings").arg(HardcodedSettings::instance().serverUrl()));
    addItem(desc_);

    proxySettingsGroup_ = new ProxySettingsGroup(this);
    proxySettingsGroup_->setProxySettings(preferences->proxySettings());
    connect(proxySettingsGroup_, &ProxySettingsGroup::proxySettingsChanged, this, &ProxySettingsWindowItem::onProxySettingsChanged);
    addItem(proxySettingsGroup_);
}

QString ProxySettingsWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Proxy Settings");
}

void ProxySettingsWindowItem::onProxySettingsPreferencesChanged(const types::ProxySettings &ps)
{
    proxySettingsGroup_->setProxySettings(ps);
}

void ProxySettingsWindowItem::onProxySettingsChanged(const types::ProxySettings &ps)
{
    preferences_->setProxySettings(ps);
}


} // namespace PreferencesWindow
