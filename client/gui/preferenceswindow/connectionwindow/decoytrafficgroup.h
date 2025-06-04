#pragma once

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/decoytrafficsettings.h"
#include "preferenceswindow/statictext.h"

#include <QVariantAnimation>

namespace PreferencesWindow {

class DecoyTrafficGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit DecoyTrafficGroup(ScalableGraphicsObject *parent,
                               const QString &desc = "",
                               const QString &descUrl = "");

    void setDecoyTrafficSettings(const types::DecoyTrafficSettings &decoyTrafficSettings);

    void setDescription(const QString &desc, const QString &descUrl = "");

signals:
    void decoyTrafficSettingsChanged(const types::DecoyTrafficSettings &decoyTrafficSettings);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onDecoyTrafficOptionChanged(QVariant v);
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    void updateMode();
    void updateStaticText();

    ToggleItem *checkBoxEnable_;
    ComboBoxItem *comboBox_;
    StaticText *staticText_;
    types::DecoyTrafficSettings decoyTrafficSettings_;
    QString textPerHour_;
};

}
