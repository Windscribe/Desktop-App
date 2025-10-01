#pragma once

#include "commongraphics/baseitem.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class ProxySettingsGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit ProxySettingsGroup(ScalableGraphicsObject *parent,
                                const QString &desc = "",
                                const QString &descUrl = "");

    void setProxySettings(const types::ProxySettings &ps);

signals:
    void proxySettingsChanged(const types::ProxySettings &ps);

private slots:
    void onProxyTypeChanged(QVariant v);

    void onAddressChanged(const QString &text);
    void onPortChanged(const QString &text);
    void onUsernameChanged(const QString &text);
    void onPasswordChanged(const QString &text);

    void onLanguageChanged();

private:
    void updateMode();

    ComboBoxItem *comboBoxProxyType_;
    types::ProxySettings settings_;
    EditBoxItem *editBoxAddress_;
    EditBoxItem *editBoxPort_;
    EditBoxItem *editBoxUsername_;
    EditBoxItem *editBoxPassword_;
};

} // namespace PreferencesWindow
