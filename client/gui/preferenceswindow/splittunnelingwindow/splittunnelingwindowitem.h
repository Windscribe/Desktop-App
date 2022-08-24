#ifndef SPLITTUNNELINGWINDOWITEM_H
#define SPLITTUNNELINGWINDOWITEM_H

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "splittunnelinggroup.h"

enum SPLIT_TUNNEL_SCREEN { SPLIT_TUNNEL_SCREEN_HOME, SPLIT_TUNNEL_SCREEN_APPS, SPLIT_TUNNEL_SCREEN_APPS_SEARCH, SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES };

namespace PreferencesWindow {

class SplitTunnelingWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();

    SPLIT_TUNNEL_SCREEN getScreen();
    void setScreen(SPLIT_TUNNEL_SCREEN screen);

    void setAppsCount(int count);
    void setNetworkRoutesCount(int count);
    void setLoggedIn(bool loggedIn);

signals:
    void appsPageClick();
    void addressesPageClick();

private slots:
    void onSettingsChanged(types::SplitTunnelingSettings settings);

private:
    SPLIT_TUNNEL_SCREEN currentScreen_;
    Preferences *preferences_;
    PreferenceGroup *desc_;
    QString descText_;
    SplitTunnelingGroup *splitTunnelingGroup_;
};

} // namespace PreferencesWindow

#endif // SPLITTUNNELINGWINDOWITEM_H
