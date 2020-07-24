#ifndef SPLITTUNNELINGWINDOWITEM_H
#define SPLITTUNNELINGWINDOWITEM_H

#include "../basepage.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "../checkboxitem.h"
#include "splittunnelingitem.h"

enum SPLIT_TUNNEL_SCREEN { SPLIT_TUNNEL_SCREEN_HOME, SPLIT_TUNNEL_SCREEN_APPS, SPLIT_TUNNEL_SCREEN_APPS_SEARCH, SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES };

namespace PreferencesWindow {

class SplitTunnelingWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();

    SPLIT_TUNNEL_SCREEN getScreen();
    void setScreen(SPLIT_TUNNEL_SCREEN screen);

    void setAppsCount(int count);
    void setNetworkRoutesCount(int count);

signals:
    void appsPageClick();
    void ipsAndHostnamesPageClick();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onSettingsChanged(ProtoTypes::SplitTunnelingSettings settings);

private:
    SPLIT_TUNNEL_SCREEN currentScreen_;
    Preferences *preferences_;
    SplitTunnelingItem *splitTunnelingItem_;
};

} // namespace PreferencesWindow

#endif // SPLITTUNNELINGWINDOWITEM_H
