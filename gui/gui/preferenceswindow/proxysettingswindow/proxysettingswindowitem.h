#ifndef PROXYSETTINGSWINDOWITEM_H
#define PROXYSETTINGSWINDOWITEM_H

#include "../basepage.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "proxysettingsitem.h"

namespace PreferencesWindow {

class ProxySettingsWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit ProxySettingsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();

private slots:
    void onProxySettingsPreferencesChanged(const ProtoTypes::ProxySettings &ps);
    void onProxySettingsChanged(const ProtoTypes::ProxySettings &ps);

private:
    Preferences *preferences_;
    ProxySettingsItem *proxySettingsItem_;

};

} // namespace PreferencesWindow

#endif // PROXYSETTINGSWINDOWITEM_H
