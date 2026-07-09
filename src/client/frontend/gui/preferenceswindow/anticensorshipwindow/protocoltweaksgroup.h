#pragma once

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/enums.h"

namespace PreferencesWindow {

class ProtocolTweaksGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit ProtocolTweaksGroup(ScalableGraphicsObject *parent,
                                  const QString &desc = "",
                                  const QString &descUrl = "");

    void setProtocolTweaksMethod(PROTOCOL_TWEAKS_METHOD_TYPE method);
    void setAmneziawgPreset(const QString &preset);
    void setAmneziawgPresets(const QStringList &presets);
    void setCustomSniDomain(const QString &domain);
    QString customSniDomain() const;

    void setDescription(const QString &desc, const QString &descUrl = "");

signals:
    void protocolTweaksMethodChanged(PROTOCOL_TWEAKS_METHOD_TYPE method);
    void amneziawgPresetChanged(QString preset);
    void customSniDomainChanged(const QString &domain);

private slots:
    void onComboBoxModeChanged(const QVariant &value);
    void onAmneziawgPresetChanged(const QVariant &value);
    void onCustomSniDomainChanged(const QString &domain);
    void onLanguageChanged();

private:
    void updateMode();
    void setAmneziaIsUserConfigured();

    ComboBoxItem *comboBoxMode_;
    ComboBoxItem *amneziawgPreset_;
    EditBoxItem *customSniDomainEdit_;

    PROTOCOL_TWEAKS_METHOD_TYPE method_ = PROTOCOL_TWEAKS_METHOD_AUTO;
};

} // namespace PreferencesWindow
