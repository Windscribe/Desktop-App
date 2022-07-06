#ifndef SPLITTUNNELINGGROUP_H
#define SPLITTUNNELINGGROUP_H

#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"

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

signals:
    void settingsChanged(types::SplitTunnelingSettings settings);
    void appsPageClick();
    void addressesPageClick();

private slots:
    void onActiveSwitchChanged(bool checked);
    void onCurrentModeChanged(QVariant value);

private:
    void updateDescription();

    types::SplitTunnelingSettings settings_;

    CheckBoxItem *activeCheckBox_;
    ComboBoxItem *modeComboBox_;
    LinkItem *appsLinkItem_;
    LinkItem *addressesLinkItem_;

    QVariantAnimation expandAnimation_;
};

} // namespace PreferencesWindow

#endif // SPLITTUNNELINGGROUP_H
