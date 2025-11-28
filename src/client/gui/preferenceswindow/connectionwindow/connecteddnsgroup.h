#pragma once

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/verticaleditboxitem.h"
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
    void setLocalDnsAvailable(bool available);
    bool hasItemWithFocus() override;

    void setDescription(const QString &desc, const QString &descUrl = "");

public slots:
    void onControldDevicesFetched(CONTROLD_FETCH_RESULT result, const QList<QPair<QString, QString>> &devices);

signals:
    void connectedDnsInfoChanged(const types::ConnectedDnsInfo &dns);
    void domainsClick(const QStringList &domains);
    void fetchControldDevices(const QString &apiKey);

private slots:
    void onConnectedDnsModeChanged(QVariant v);
    void onControldApiKeyChanged(QString v);
    void onControldApiKeyRefreshClick();
    void onControldDeviceChanged(QVariant v);
    void onUpstream1Changed(QString v);
    void onUpstream2Changed(QString v);
    void onSplitDnsStateChanged(bool checked);
    void onDomainsClick();
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    void updateMode();
    void checkDnsLeak(const QString &v1, const QString &v2 = "");
    void populateDnsTypes(bool isLocalDnsAvailable);
    void fetchDevices(const QString &apiKey);

    ComboBoxItem *comboBoxDns_;
    VerticalEditBoxItem *editBoxControldApiKey_;
    ComboBoxItem *comboBoxControldDevice_;
    VerticalEditBoxItem *editBoxUpstream1_;
    VerticalEditBoxItem *editBoxUpstream2_;
    ToggleItem *splitDnsCheckBox_;
    LinkItem *domainsItem_;

    types::ConnectedDnsInfo settings_;
    bool isLocalDnsAvailable_;
    bool isControldRequestInProgress_;
    CONTROLD_FETCH_RESULT controldLastFetchResult_;
};

}
