#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "preferenceswindow/preferencegroup.h"
#include "proxysettingsgroup.h"

namespace PreferencesWindow {

class ProxySettingsWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;

private slots:
    void onProxySettingsPreferencesChanged(const types::ProxySettings &ps);
    void onProxySettingsChanged(const types::ProxySettings &ps);
    void onLanguageChanged();

private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    ProxySettingsGroup *proxySettingsGroup_;

};

} // namespace PreferencesWindow
