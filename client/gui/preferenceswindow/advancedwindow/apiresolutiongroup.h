#pragma once

#include "commongraphics/baseitem.h"

#include <QVariantAnimation>
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/apiresolutionsettings.h"

namespace PreferencesWindow {

class ApiResolutionGroup: public PreferenceGroup
{
    Q_OBJECT
public:
    explicit ApiResolutionGroup(ScalableGraphicsObject *parent,
                                const QString &desc = "",
                                const QString &descUrl = "");

    void setApiResolution(const types::ApiResolutionSettings &ar);
    bool hasItemWithFocus() override;

    void setDescription(const QString &desc, const QString &url = "");

signals:
    void apiResolutionChanged(const types::ApiResolutionSettings &ar);

private slots:
    void onAutomaticChanged(QVariant value);
    void onAddressChanged(const QString &text);
    void onLanguageChanged();

private:
    void updateMode();
    ComboBoxItem *resolutionModeItem_;
    EditBoxItem *editBoxAddress_;
    types::ApiResolutionSettings settings_;
};

} // namespace PreferencesWindow
