#include "splittunnelingwindowitem.h"

#include <QPainter>
#include "utils/hardcodedsettings.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingWindowItem::SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : CommonGraphics::BasePage(parent)
    , currentScreen_(SPLIT_TUNNEL_SCREEN_HOME)
    , preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    splitTunnelingGroup_ = new SplitTunnelingGroup(this);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::addressesPageClick, this, &SplitTunnelingWindowItem::addressesPageClick);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::appsPageClick, this, &SplitTunnelingWindowItem::appsPageClick);
    connect(splitTunnelingGroup_, &SplitTunnelingGroup::settingsChanged, this, &SplitTunnelingWindowItem::onSettingsChanged);

    splitTunnelingGroup_->setSettings(preferences->splitTunnelingSettings());
    splitTunnelingGroup_->setAppsCount(preferences->splitTunnelingApps().count());
    splitTunnelingGroup_->setAddressesCount(preferences->splitTunnelingNetworkRoutes().count());

    QString descText = tr("Include or exclude apps and hostnames from the VPN tunnel.");
#ifdef Q_OS_MAC
    if (MacUtils::isOsVersionIsBigSur_or_greater())
    {
        descText = tr("Sorry, Split Tunneling is currently not supported on your operating system.  We are working hard to enable the feature as soon as possible.");
        splitTunnelingGroup_->setEnabled(false);
    }
    else
    {
        descText = tr("Include or exclude apps and hostnames from the VPN tunnel.\n\nFirewall will not function in this mode.");
    }
#endif
    desc_ = new PreferenceGroup(this,
                                descText,
                                QString("https://%1/features/split-tunneling").arg(HardcodedSettings::instance().serverUrl()));

    addItem(desc_);
    addItem(splitTunnelingGroup_);
}

QString SplitTunnelingWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Split Tunneling");
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

} // namespace PreferencesWindow
