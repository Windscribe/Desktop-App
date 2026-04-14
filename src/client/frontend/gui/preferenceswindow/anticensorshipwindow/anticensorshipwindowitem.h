#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/preferencegroup.h"
#include "protocoltweaksgroup.h"
#include "types/enums.h"

namespace PreferencesWindow {

class AntiCensorshipWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit AntiCensorshipWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;
    void updateScaling() override;

    void setAmneziawgUnblockParams(const QString &activePreset, QStringList presets);

private slots:
    void onLanguageChanged();

    void onAntiCensorshipPreferencesChanged(bool b);
    void onAmneziawgPresetChanged(const QString &preset);
    void onServerRoutingMethodPreferencesChanged(SERVER_ROUTING_METHOD_TYPE method);

    // slots for changes made by user
    void onAntiCensorshipPreferencesChangedByUser(bool isChecked);
    void onAmneziawgPresetChangedByUser(const QString &preset);
    void onServerRoutingMethodChangedByUser(QVariant method);

private:
    Preferences *preferences_;

    PreferenceGroup *desc_;
    ProtocolTweaksGroup *protocolTweaksGroup_;
    PreferenceGroup *serverRoutingGroup_;
    ComboBoxItem *comboBoxServerRouting_;
};

} // namespace PreferencesWindow
