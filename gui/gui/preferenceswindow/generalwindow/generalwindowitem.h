#ifndef GENERALWINDOWITEM_H
#define GENERALWINDOWITEM_H

#include "../basepage.h"
#include "../checkboxitem.h"
#include "../comboboxitem.h"
#include "versioninfoitem.h"
#include "../backend/preferences/preferenceshelper.h"
#include "../backend/preferences/preferences.h"
#include "ipc/generated_proto/types.pb.h"

namespace PreferencesWindow {

class GeneralWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit GeneralWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();

    virtual void updateScaling();

private slots:
    void onIsLaunchOnStartupClicked(bool isChecked);
    void onIsLaunchOnStartupPreferencesChanged(bool b);

    void onIsAutoConnectClicked(bool isChecked);
    void onIsAutoConnectPreferencesChanged(bool b);

#ifdef Q_OS_WIN
    void onMinimizeAndCloseToTrayPreferencesChanged(bool b);
    void onMinimizeAndCloseToTrayClicked(bool b);
#elif defined Q_OS_MAC
    void onHideFromDockPreferecesChanged(bool b);
    void onHideFromDockClicked(bool b);
#endif
    void onIsShowNotificationsPreferencesChanged(bool b);
    void onIsShowNotificationsClicked(bool b);

    void onIsDockedToTrayPreferencesChanged(bool b);
    void onDockedToTrayChanged(bool b);

    void onLanguagePreferencesChanged(const QString &lang);
    void onLanguageItemChanged(QVariant lang);

    void onLocationOrderPreferencesChanged(ProtoTypes::OrderLocationType o);
    void onLocationItemChanged(QVariant o);

    void onLatencyDisplayPreferencesChanged(ProtoTypes::LatencyDisplayType l);
    void onLatencyItemChanged(QVariant o);

    void onUpdateChannelPreferencesChanged(ProtoTypes::UpdateChannel c);
    void onUpdateChannelItemChanged(QVariant o);

    void onLanguageChanged();

signals:
    void languageChanged();

protected:
    void hideOpenPopups();

private:
    Preferences *preferences_;

    CheckBoxItem *checkBoxLaunchOnStart_;
    CheckBoxItem *checkBoxAutoConnect_;
    CheckBoxItem *checkBoxAllowLanTraffic_;
    CheckBoxItem *checkBoxShowNotifications_;
    CheckBoxItem *checkBoxDockedToTray_;

#ifdef Q_OS_WIN
    CheckBoxItem *checkBoxMinimizeAndCloseToTray_;
#elif defined Q_OS_MAC
    CheckBoxItem *checkBoxHideFromDock_;
#endif

    ComboBoxItem *comboBoxLanguage_;
    ComboBoxItem *comboBoxLocationOrder_;
    ComboBoxItem *comboBoxLatencyDisplay_;
    ComboBoxItem *comboBoxUpdateChannel_;

    VersionInfoItem *versionInfoItem_;
};

} // namespace PreferencesWindow

#endif // GENERALWINDOWITEM_H
