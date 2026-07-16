#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/preferencegroup.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/verticaleditboxitem.h"
#include "backend/preferences/preferenceshelper.h"
#include "protocoltweaksgroup.h"
#include "types/enums.h"

namespace PreferencesWindow {

class AntiCensorshipWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit AntiCensorshipWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;
    void updateScaling() override;

private slots:
    void onLanguageChanged();

    void onProtocolTweaksMethodPreferencesChanged(PROTOCOL_TWEAKS_METHOD_TYPE method);
    void onAmneziawgPresetChanged(const QString &preset);
    void onAmneziawgPresetsChanged(const QStringList &presets);
    void onCustomSniDomainPreferencesChanged(const QString &domain);
    void onApiSuggestedCustomSniChanged(const QString &domain);
    void onServerRoutingMethodPreferencesChanged(SERVER_ROUTING_METHOD_TYPE method);
    void onIsAPIAntiCensorshipPreferencesChanged(bool b);

    // slots for changes made by user
    void onProtocolTweaksMethodChangedByUser(PROTOCOL_TWEAKS_METHOD_TYPE method);
    void onAmneziawgPresetChangedByUser(const QString &preset);
    void onCustomSniDomainChangedByUser(const QString &domain);
    void onServerRoutingMethodChangedByUser(QVariant method);
    void onLargeTlsToggleChangedByUser(bool isChecked);

private:
    void applyAmneziawgPresets(const QStringList &presets);

    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    PreferenceGroup *desc_;
    ProtocolTweaksGroup *protocolTweaksGroup_;
    PreferenceGroup *serverRoutingGroup_;
    ComboBoxItem *comboBoxServerRouting_;
    PreferenceGroup *largeTlsGroup_;
    ToggleItem *toggleLargeTls_;
    PreferenceGroup *customSniGroup_;
    VerticalEditBoxItem *customSniDomainEdit_;
};

} // namespace PreferencesWindow
