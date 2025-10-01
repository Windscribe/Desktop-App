#pragma once

#include "mixedcomboboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/soundsettings.h"

namespace PreferencesWindow {

class SoundsGroup: public PreferenceGroup
{
    Q_OBJECT
public:
    explicit SoundsGroup(ScalableGraphicsObject *parent,
                         const QString &desc = "",
                         const QString &descUrl = "");
    void setSoundSettings(const types::SoundSettings &settings);

    void hideMenu();
    void setDescription(const QString &desc, const QString &url = "");

signals:
    void soundSettingsChanged(const types::SoundSettings &settings);

private slots:
    void onDisconnectedComboBoxChanged(const QVariant &v);
    void onDisconnectedPathChanged(const QVariant &v);
    void onConnectedComboBoxChanged(const QVariant &v);
    void onConnectedPathChanged(const QVariant &v);
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    LinkItem *titleItem_;
    MixedComboBoxItem *disconnectedComboBox_;
    MixedComboBoxItem *connectedComboBox_;

    types::SoundSettings settings_;

    QStringList soundNames_;

    QStringList enumerateSounds();
    QString displayName(const QString &sound) const;
    QString filename(const QString &sound, bool connected) const;
};

} // namespace PreferencesWindow
