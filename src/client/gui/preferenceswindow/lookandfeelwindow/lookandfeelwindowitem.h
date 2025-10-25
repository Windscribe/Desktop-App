#pragma once

#include "appbackgroundgroup.h"
#include "commongraphics/basepage.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "soundsgroup.h"

namespace PreferencesWindow {

class LookAndFeelWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit LookAndFeelWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;
    void updateScaling() override;

    void setLocationNamesImportCompleted();

private slots:
    void onBackgroundSettingsChanged(const types::BackgroundSettings &settings);
    void onPreferencesBackgroundSettingsChanged(const types::BackgroundSettings &settings);

    void onSoundSettingsChanged(const types::SoundSettings &settings);
    void onPreferencesSoundSettingsChanged(const types::SoundSettings &settings);

    void onLanguageChanged();

    void onAppSkinPreferencesChanged(APP_SKIN s);
    void onAppSkinChanged(QVariant value);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    void onTrayIconColorChanged(QVariant value);
    void onPreferencesTrayIconColorChanged(QVariant value);
#endif

    void onResetLocationNamesClicked();

signals:
    void exportLocationNamesClick();
    void importLocationNamesClick();
    void resetLocationNamesClick();

protected:
    void hideOpenPopups() override;

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    PreferenceGroup *appSkinGroup_;
    ComboBoxItem *appSkinItem_;
    AppBackgroundGroup *appBackgroundGroup_;
    SoundsGroup *soundsGroup_;
    PreferenceGroup *renameLocationsGroup_;
    LinkItem *renameLocationsItem_;
    LinkItem *exportSettingsItem_;
    LinkItem *importSettingsItem_;
    LinkItem *resetLocationsItem_;
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    PreferenceGroup *trayIconColorGroup_;
    ComboBoxItem *trayIconColorItem_;
#endif
};

} // namespace PreferencesWindow
