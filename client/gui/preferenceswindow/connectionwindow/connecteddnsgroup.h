#ifndef CONNECTEDDNSITEM_H
#define CONNECTEDDNSITEM_H

#include "commongraphics/baseitem.h"
#include "backend/preferences/preferences.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/connecteddnsinfo.h"

#include <QVariantAnimation>

namespace PreferencesWindow {

class ConnectedDnsGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit ConnectedDnsGroup(ScalableGraphicsObject *parent,
                               const QString &desc = "",
                               const QString &descUrl = "");

    void setConnectedDnsInfo(const types::ConnectedDnsInfo &dns);
    bool hasItemWithFocus() override;

signals:
    void connectedDnsInfoChanged(const types::ConnectedDnsInfo &dns);

private slots:
    void onConnectedDnsModeChanged(QVariant v);
    void onConnectedDnsIpChanged(QString v);
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    void updateMode();

    ComboBoxItem *comboBoxDns_;
    EditBoxItem *editBoxIp_;

    types::ConnectedDnsInfo settings_;
};

}
#endif // CONNECTEDDNSITEM_H
