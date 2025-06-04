#pragma once

#include <QGraphicsObject>
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

class FirewallGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit FirewallGroup(ScalableGraphicsObject *parent,
                           const QString &desc = "",
                           const QString &descUrl = "");

    void setFirewallSettings(types::FirewallSettings settings);
    void setDescription(const QString &desc, const QString &descUrl);

signals:
    void firewallPreferencesChanged(const types::FirewallSettings &settings);

private slots:
    void onFirewallModeChanged(QVariant value);
    void onFirewallWhenChanged(QVariant value);
    void onLanguageChanged();

private:
    void updateMode();
    types::FirewallSettings settings_;
    ComboBoxItem *firewallModeItem_;
    ComboBoxItem *firewallWhenItem_;
};

} // namespace PreferencesWindow
