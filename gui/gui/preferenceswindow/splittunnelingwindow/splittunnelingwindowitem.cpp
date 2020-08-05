#include "splittunnelingwindowitem.h"

#include <QPainter>

namespace PreferencesWindow {

SplitTunnelingWindowItem::SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences) : BasePage(parent)
    , currentScreen_(SPLIT_TUNNEL_SCREEN_HOME)
    , preferences_(preferences)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    splitTunnelingItem_ = new SplitTunnelingItem(this);
    connect(splitTunnelingItem_, SIGNAL(ipsAndHostnamesPageClick()), SIGNAL(ipsAndHostnamesPageClick()));
    connect(splitTunnelingItem_, SIGNAL(appsPageClick()), SIGNAL(appsPageClick()));
    connect(splitTunnelingItem_, SIGNAL(settingsChanged(ProtoTypes::SplitTunnelingSettings)), SLOT(onSettingsChanged(ProtoTypes::SplitTunnelingSettings)));
    connect(splitTunnelingItem_, SIGNAL(showTooltip(TooltipInfo)),SIGNAL(showTooltip(TooltipInfo)));
    connect(splitTunnelingItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));

    addItem(splitTunnelingItem_);
    splitTunnelingItem_->setSettings(preferences->splitTunnelingSettings());

    splitTunnelingItem_->setAppsCount(preferences->splitTunnelingApps().count());
    splitTunnelingItem_->setIpsAndHostnamesCount(preferences->splitTunnelingNetworkRoutes().count());
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
    splitTunnelingItem_->setAppsCount(count);
}

void SplitTunnelingWindowItem::setNetworkRoutesCount(int count)
{
    splitTunnelingItem_->setIpsAndHostnamesCount(count);
}

void SplitTunnelingWindowItem::onSettingsChanged(ProtoTypes::SplitTunnelingSettings settings)
{
    preferences_->setSplitTunnelingSettings(settings);
}

} // namespace PreferencesWindow
