#pragma once

#include <QGraphicsObject>
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"

namespace PreferencesWindow {

class ProtocolGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    enum SelectionType {
        COMBO_BOX,
        TOGGLE_SWITCH,
    };

    explicit ProtocolGroup(ScalableGraphicsObject *parent,
                           PreferencesHelper *preferencesHelper,
                           const QString &title,
                           const QString &path,
                           const SelectionType type = SelectionType::COMBO_BOX,
                           const QString &desc = "",
                           const QString &descUrl = "");

    void setConnectionSettings(const types::ConnectionSettings &cm);
    void setTitle(const QString &title);
    void setDescription(const QString &description, const QString &descUrl = "");

signals:
    void connectionModePreferencesChanged(const types::ConnectionSettings &settings);

private slots:
    void onPortMapChanged();
    void onAutomaticChanged(QVariant value);
    void onCheckBoxStateChanged(bool isChecked);
    void onProtocolChanged(QVariant value);
    void onPortChanged(QVariant value);
    void onLanguageChanged();

private:
    void updatePositions();
    void updatePorts(types::Protocol protocol);
    void updateMode();
    PreferencesHelper *preferencesHelper_;

    QString title_;
    ComboBoxItem *connectionModeItem_;
    ToggleItem *checkBoxEnable_;
    ComboBoxItem *protocolItem_;
    ComboBoxItem *portItem_;

    SelectionType type_;

    bool isPortMapInitialized_;

    types::ConnectionSettings settings_;
};

} // namespace PreferencesWindow
