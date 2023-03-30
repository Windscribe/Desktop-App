#ifndef CONNECTEDDNSITEM_H
#define CONNECTEDDNSITEM_H

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/linkitem.h"
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
    void domainsClick(const QStringList &domains);

private slots:
    void onConnectedDnsModeChanged(QVariant v);
    void onUpstream1Changed(QString v);
    void onUpstream2Changed(QString v);
    void onSplitDnsStateChanged(bool checked);
    void onDomainsClick();
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    void updateMode();

    ComboBoxItem *comboBoxDns_;
    EditBoxItem *editBoxUpstream1_;
    EditBoxItem *editBoxUpstream2_;
    CheckBoxItem *splitDnsCheckBox_;
    LinkItem *domainsItem_;

    types::ConnectedDnsInfo settings_;
};

}
#endif // CONNECTEDDNSITEM_H
