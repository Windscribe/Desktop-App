#ifndef PROXYSETTINGSWINDOWITEM_H
#define PROXYSETTINGSWINDOWITEM_H

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

    QString caption();

private slots:
    void onProxySettingsPreferencesChanged(const types::ProxySettings &ps);
    void onProxySettingsChanged(const types::ProxySettings &ps);

private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    ProxySettingsGroup *proxySettingsGroup_;

};

} // namespace PreferencesWindow

#endif // PROXYSETTINGSWINDOWITEM_H
