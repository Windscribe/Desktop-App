#pragma once

#include <QMutex>
#include <QNetworkInformation>

#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"
#include "routemonitor_linux.h"

class NetworkDetectionManager_linux : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_linux(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_linux() override;
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate = false) override;
    bool isOnline() override;

signals:
    void networkListChanged(const QList<types::NetworkInterface> &interfaces);

private slots:
    void onRoutesChanged();

private:
    bool isOnline_ = false;
    types::NetworkInterface networkInterface_;

    QThread *routeMonitorThread_ = nullptr;
    RouteMonitor_linux *routeMonitor_ = nullptr;

    void updateNetworkInfo(bool bWithEmitSignal);
    QString getDefaultRouteInterface(bool &isOnline);
};
