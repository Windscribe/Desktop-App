#include "proxysettingswindowitem.h"

#include <QPainter>
#include "languagecontroller.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

ProxySettingsWindowItem::ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : CommonGraphics::BasePage(parent),
    preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    connect(preferences, &Preferences::proxySettingsChanged, this, &ProxySettingsWindowItem::onProxySettingsPreferencesChanged);

    desc_ = new PreferenceGroup(this,
                                "",
                                QString("https://%1/features/proxy-settings").arg(HardcodedSettings::instance().windscribeServerUrl()));
    addItem(desc_);

    proxySettingsGroup_ = new ProxySettingsGroup(this);
    proxySettingsGroup_->setProxySettings(preferences->proxySettings());
    connect(proxySettingsGroup_, &ProxySettingsGroup::proxySettingsChanged, this, &ProxySettingsWindowItem::onProxySettingsChanged);
    addItem(proxySettingsGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProxySettingsWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString ProxySettingsWindowItem::caption() const
{
    return tr("Proxy Settings");
}

void ProxySettingsWindowItem::onProxySettingsPreferencesChanged(const types::ProxySettings &ps)
{
    proxySettingsGroup_->setProxySettings(ps);
}

void ProxySettingsWindowItem::onProxySettingsChanged(const types::ProxySettings &ps)
{
    preferences_->setProxySettings(ps);
}

void ProxySettingsWindowItem::onLanguageChanged()
{
    desc_->setDescription(
        tr("If your network has a LAN proxy, configure it here. Please note these proxy settings are only utilized when connecting with the TCP protocol."));
}

} // namespace PreferencesWindow
