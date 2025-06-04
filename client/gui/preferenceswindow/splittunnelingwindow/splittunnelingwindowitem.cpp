#include "splittunnelingwindowitem.h"

#include <QPainter>

#include "utils/hardcodedsettings.h"
#include "languagecontroller.h"

namespace PreferencesWindow {

SplitTunnelingWindowItem::SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : CommonGraphics::BasePage(parent)
    , currentScreen_(SPLIT_TUNNEL_SCREEN_HOME)
    , preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    splitTunnelingGroup_ = new SplitTunnelingGroup(this);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::addressesPageClick, this, &SplitTunnelingWindowItem::addressesPageClick);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::appsPageClick, this, &SplitTunnelingWindowItem::appsPageClick);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::settingsChanged, this, &SplitTunnelingWindowItem::onSettingsChanged);

    connect(preferences_, &Preferences::splitTunnelingChanged, this, &SplitTunnelingWindowItem::onPreferencesChanged);
    onPreferencesChanged();

    desc_ = new PreferenceGroup(this,
                                "",
                                QString("https://%1/features/split-tunneling/desktop").arg(HardcodedSettings::instance().windscribeServerUrl()));

    addItem(desc_);
    addItem(splitTunnelingGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SplitTunnelingWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString SplitTunnelingWindowItem::caption() const
{
    return tr("Split Tunneling");
}

SPLIT_TUNNEL_SCREEN SplitTunnelingWindowItem::getScreen()
{
    return currentScreen_;
}

void SplitTunnelingWindowItem::setScreen(SPLIT_TUNNEL_SCREEN screen)
{
    currentScreen_ = screen;
}

void SplitTunnelingWindowItem::setAppsCount(int count)
{
    splitTunnelingGroup_->setAppsCount(count);
}

void SplitTunnelingWindowItem::setNetworkRoutesCount(int count)
{
    splitTunnelingGroup_->setAddressesCount(count);
}

void SplitTunnelingWindowItem::onSettingsChanged(types::SplitTunnelingSettings settings)
{
    preferences_->setSplitTunnelingSettings(settings);
}

void SplitTunnelingWindowItem::setActive(bool active)
{
    splitTunnelingGroup_->setActive(active);
}

void SplitTunnelingWindowItem::onLanguageChanged()
{
    desc_->setDescription(tr("Include or exclude apps and hostnames from the VPN tunnel."));
}

void SplitTunnelingWindowItem::onPreferencesChanged()
{
    splitTunnelingGroup_->setSettings(preferences_->splitTunnelingSettings());
    splitTunnelingGroup_->setAppsCount(preferences_->splitTunnelingApps().count());
    splitTunnelingGroup_->setAddressesCount(preferences_->splitTunnelingNetworkRoutes().count());
}

} // namespace PreferencesWindow
