#pragma once

#include "backend/preferences/preferences.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/preferencegroup.h"
#include "splittunnelinggroup.h"

enum SPLIT_TUNNEL_SCREEN { SPLIT_TUNNEL_SCREEN_HOME, SPLIT_TUNNEL_SCREEN_APPS, SPLIT_TUNNEL_SCREEN_APPS_SEARCH, SPLIT_TUNNEL_SCREEN_IPS_AND_HOSTNAMES };

namespace PreferencesWindow {

class SplitTunnelingWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;

    SPLIT_TUNNEL_SCREEN getScreen();
    void setScreen(SPLIT_TUNNEL_SCREEN screen);

    void setAppsCount(int count);
    void setNetworkRoutesCount(int count);
    void setLoggedIn(bool loggedIn);
    void setActive(bool active);
    void setSystemExtensionAvailability(bool available);

signals:
    void appsPageClick();
    void addressesPageClick();

private slots:
    void onSettingsChanged(types::SplitTunnelingSettings settings);
    void onLanguageChanged();
    void onPreferencesChanged();

private:
    SPLIT_TUNNEL_SCREEN currentScreen_;
    Preferences *preferences_;
    PreferenceGroup *desc_;
    QString descText_;
    SplitTunnelingGroup *splitTunnelingGroup_;
    bool systemExtensionAvailable_;
};

} // namespace PreferencesWindow
