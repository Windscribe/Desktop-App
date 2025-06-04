#pragma once

#include "mixedcomboboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/backgroundsettings.h"

namespace PreferencesWindow {

class AppBackgroundGroup: public PreferenceGroup
{
    Q_OBJECT
public:
    explicit AppBackgroundGroup(ScalableGraphicsObject *parent,
                                const QString &desc = "",
                                const QString &descUrl = "");
    void setBackgroundSettings(const types::BackgroundSettings &settings);

    void hideMenu();
    void setDescription(const QString &desc, const QString &url = "");

signals:
    void backgroundSettingsChanged(const types::BackgroundSettings &settings);

private slots:
    void onBackgroundAspectRatioModeChanged(const QVariant &v);
    void onDisconnectedComboBoxChanged(const QVariant &v);
    void onDisconnectedPathChanged(const QVariant &v);
    void onConnectedComboBoxChanged(const QVariant &v);
    void onConnectedPathChanged(const QVariant &v);
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    void updateMode();

    LinkItem *titleItem_;
    ComboBoxItem *comboBoxAspectRatioMode_;
    MixedComboBoxItem *disconnectedComboBox_;
    MixedComboBoxItem *connectedComboBox_;

    types::BackgroundSettings settings_;
};

} // namespace PreferencesWindow
