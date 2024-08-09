#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "backgroundsettingsgroup.h"
#include "versioninfoitem.h"

namespace PreferencesWindow {

class GeneralWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit GeneralWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;
    void updateScaling() override;

private slots:
    void onIsLaunchOnStartupClicked(bool isChecked);
    void onIsLaunchOnStartupPreferencesChanged(bool b);

    void onStartMinimizedPreferencesChanged(bool b);
    void onStartMinimizedClicked(bool b);
    void onVersionInfoClicked();

    void onMinimizeAndCloseToTrayPreferencesChanged(bool b);
    void onMinimizeAndCloseToTrayClicked(bool b);

#if defined Q_OS_MACOS
    void onHideFromDockPreferecesChanged(bool b);
    void onHideFromDockClicked(bool b);
#endif
    void onIsShowNotificationsPreferencesChanged(bool b);
    void onIsShowNotificationsClicked(bool b);

    ///void onIsShowCountryFlagsPreferencesChanged(bool b);
    ///void onIsShowCountryFlagsClicked(bool b);

    void onIsDockedToTrayPreferencesChanged(bool b);
    void onDockedToTrayChanged(bool b);

    void onLanguagePreferencesChanged(const QString &lang);
    void onLanguageItemChanged(QVariant lang);

    void onLocationOrderPreferencesChanged(ORDER_LOCATION_TYPE o);
    void onLocationItemChanged(QVariant o);

    void onLatencyDisplayPreferencesChanged(LATENCY_DISPLAY_TYPE l);
    void onLatencyItemChanged(QVariant o);

    void onUpdateChannelPreferencesChanged(const UPDATE_CHANNEL &c);
    void onUpdateChannelItemChanged(QVariant o);

    void onBackgroundSettingsChanged(const types::BackgroundSettings &settings);
    void onPreferencesBackgroundSettingsChanged(const types::BackgroundSettings &settings);

    void onLanguageChanged();

    void onShowLocationLoadPreferencesChanged(bool b);
    void onShowLocationLoadClicked(bool b);

    void onAppSkinPreferencesChanged(APP_SKIN s);
    void onAppSkinChanged(QVariant value);

#if defined(Q_OS_LINUX)
    void onTrayIconColorChanged(QVariant value);
    void onPreferencesTrayIconColorChanged(QVariant value);
#endif

signals:
    void languageChanged();

protected:
    void hideOpenPopups() override;

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    PreferenceGroup *launchOnStartGroup_;
    ToggleItem *checkBoxLaunchOnStart_;
    PreferenceGroup *showNotificationsGroup_;
    ToggleItem *checkBoxShowNotifications_;
    BackgroundSettingsGroup *backgroundSettingsGroup_;
    PreferenceGroup *dockedGroup_;
    ToggleItem *checkBoxDockedToTray_;
    PreferenceGroup *startMinimizedGroup_;
    ToggleItem *checkBoxStartMinimized_;
    PreferenceGroup *showLocationLoadGroup_;
    ToggleItem *checkBoxShowLocationLoad_;
    PreferenceGroup *closeToTrayGroup_;
    ToggleItem *checkBoxMinimizeAndCloseToTray_;
#if defined Q_OS_MACOS
    PreferenceGroup *hideFromDockGroup_;
    ToggleItem *checkBoxHideFromDock_;
#endif

    PreferenceGroup *appSkinGroup_;
    ComboBoxItem *appSkinItem_;
    PreferenceGroup *languageGroup_;
    ComboBoxItem *comboBoxLanguage_;
    PreferenceGroup *locationOrderGroup_;
    ComboBoxItem *comboBoxLocationOrder_;
    PreferenceGroup *latencyDisplayGroup_;
    ComboBoxItem *comboBoxLatencyDisplay_;
    PreferenceGroup *updateChannelGroup_;
    ComboBoxItem *comboBoxUpdateChannel_;

    PreferenceGroup *versionGroup_;
    VersionInfoItem *versionInfoItem_;

#if defined(Q_OS_LINUX)
    PreferenceGroup *trayIconColorGroup_;
    ComboBoxItem *trayIconColorItem_;
#endif
};

} // namespace PreferencesWindow
