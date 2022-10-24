#pragma once

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "selectimageitem.h"
#include "types/backgroundsettings.h"

namespace PreferencesWindow {

class BackgroundSettingsGroup: public PreferenceGroup
{
    Q_OBJECT
public:
    explicit BackgroundSettingsGroup(ScalableGraphicsObject *parent,
                                     const QString &desc = "",
                                     const QString &descUrl = "");
    void setBackgroundSettings(const types::BackgroundSettings &settings);

signals:
    void backgroundSettingsChanged(const types::BackgroundSettings &settings);

private slots:
    void onBackgroundModeChanged(QVariant v);
    void onLanguageChanged();

    void onDisconnectedPathChanged(const QString &path);
    void onConnectedPathChanged(const QString &path);

protected:
    void hideOpenPopups() override;

private:
    void updateMode();

    ComboBoxItem *comboBoxMode_;
    SelectImageItem *imageItemDisconnected_;
    SelectImageItem *imageItemConnected_;

    bool isExpanded_;

    types::BackgroundSettings settings_;
};

} // namespace PreferencesWindow
