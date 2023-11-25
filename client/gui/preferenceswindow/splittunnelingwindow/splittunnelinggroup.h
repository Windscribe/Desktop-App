#pragma once

#include "commongraphics/scalablegraphicsobject.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/splittunneling.h"

namespace PreferencesWindow {

class SplitTunnelingGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit SplitTunnelingGroup(ScalableGraphicsObject *parent,
                                 const QString &desc = "",
                                 const QString &descUrl = "");

    void setSettings(types::SplitTunnelingSettings settings);
    void setAppsCount(int count);
    void setAddressesCount(int count);
    void setLoggedIn(bool loggedIn);
    void setEnabled(bool enabled);
    void setActive(bool active);

signals:
    void settingsChanged(types::SplitTunnelingSettings settings);
    void appsPageClick();
    void addressesPageClick();

private slots:
    void onActiveSwitchStateChanged(bool checked);
    void onCurrentModeChanged(QVariant value);
    void onLanguageChanged();

private:
    void updateDescription();

    types::SplitTunnelingSettings settings_;

    ToggleItem *activeCheckBox_;
    ComboBoxItem *modeComboBox_;
    LinkItem *appsLinkItem_;
    LinkItem *addressesLinkItem_;

    QVariantAnimation expandAnimation_;

    void updateUIState(bool active);
};

} // namespace PreferencesWindow
