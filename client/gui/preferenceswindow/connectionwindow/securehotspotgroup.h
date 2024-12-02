#pragma once

#include <QVariantAnimation>
#include "backend/preferences/preferences.h"
#include "commongraphics/baseitem.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

class SecureHotspotGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    enum HOTSPOT_SUPPORT_TYPE { HOTSPOT_SUPPORTED, HOTSPOT_NOT_SUPPORTED, HOTSPOT_NOT_SUPPORTED_BY_IKEV2, HOTSPOT_NOT_SUPPORTED_BY_INCLUSIVE_SPLIT_TUNNELING };

    explicit SecureHotspotGroup(ScalableGraphicsObject *parent,
                                const QString &desc = "",
                                const QString &descUrl = "");

    void setSecureHotspotSettings(const types::ShareSecureHotspot &ss);
    void setSupported(HOTSPOT_SUPPORT_TYPE supported);

    bool hasItemWithFocus() override;

signals:
    void secureHotspotPreferencesChanged(const types::ShareSecureHotspot &ss);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onSSIDChanged(const QString &text);
    void onPasswordChanged(const QString &password);
    void onLanguageChanged();

private:
    void updateMode();
    void updateDescription();

    ToggleItem *checkBoxEnable_;
    EditBoxItem *editBoxSSID_;
    EditBoxItem *editBoxPassword_;

    types::ShareSecureHotspot settings_;

    QString desc_;

    HOTSPOT_SUPPORT_TYPE supported_;
};

} // namespace PreferencesWindow
