#pragma once

#include <QTimer>
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "tooltips/tooltiptypes.h"


enum ADVANCED_SCREEN { ADVANCED_SCREEN_HOME, ADVANCED_SCREEN_ADVANCED_PARAMETERS };

namespace PreferencesWindow {

class AdvancedWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit AdvancedWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;

    ADVANCED_SCREEN getScreen();
    void setScreen(ADVANCED_SCREEN screen);

    void setPreferencesImportCompleted();

    void updateScaling() override;

signals:
    void advParametersClick();
private slots:
    void onIgnoreSslErrorsStateChanged(bool isChecked);
    void onKeepAliveStateChanged(bool isChecked);
    void onAppInternalDnsItemChanged(QVariant dns);
#ifdef Q_OS_LINUX
    void onDnsManagerItemChanged(QVariant dns);
#endif

    void onIgnoreSslErrorsPreferencesChanged(bool b);
    void onKeepAlivePreferencesChanged(bool b);
    void onDnsPolicyPreferencesChanged(DNS_POLICY_TYPE d);
#ifdef Q_OS_LINUX
    void onDnsManagerPreferencesChanged(DNS_MANAGER_TYPE d);
#endif
    void onLanguageChanged();

signals:
    void exportSettingsClick();
    void importSettingsClick();

protected:
    void hideOpenPopups() override;

private:
    PreferenceGroup *advParametersGroup_;
    LinkItem *advParametersItem_;

    PreferenceGroup *ignoreSslErrorsGroup_;
    ToggleItem *cbIgnoreSslErrors_;
    PreferenceGroup *keepAliveGroup_;
    ToggleItem *cbKeepAlive_;
    PreferenceGroup *internalDnsGroup_;
    ComboBoxItem *comboBoxAppInternalDns_;

    PreferenceGroup *appPreferencesGroup_;
    LinkItem *appPreferencesItem_;
    LinkItem *exportSettingsItem_;
    LinkItem *importSettingsItem_;
#ifdef Q_OS_LINUX
    PreferenceGroup *dnsManagerGroup_;
    ComboBoxItem *comboBoxDnsManager_;
#endif
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    ADVANCED_SCREEN currentScreen_;
};

} // namespace PreferencesWindow
