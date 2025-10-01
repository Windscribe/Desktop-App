#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "preferenceswindow/protocolgroup.h"

namespace PreferencesWindow {

class NetworkOptionsNetworkWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit NetworkOptionsNetworkWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;

    void setCurrentNetwork(types::NetworkInterface network);
    void setNetwork(types::NetworkInterface network);

signals:
    void escape();

private slots:
    void onAutoSecureChanged(bool b);
    void onForgetClicked();
    void onNetworkWhitelistChanged(QVector<types::NetworkInterface> l);
    void onPreferredProtocolChanged(const types::ConnectionSettings &settings);
    void onLanguageChanged();

private:
    void updateForgetGroup();

    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    PreferenceGroup *desc_;
    PreferenceGroup *autoSecureGroup_;
    ToggleItem *autoSecureCheckBox_;
    ProtocolGroup *preferredProtocolGroup_;
    PreferenceGroup *forgetGroup_;
    LinkItem *forgetItem_;

    types::NetworkInterface currentNetwork_;
    types::NetworkInterface network_;
};

} // namespace PreferencesWindow
