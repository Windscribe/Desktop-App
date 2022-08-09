#ifndef NETWORKDETECTIONMANAGER_LINUX_H
#define NETWORKDETECTIONMANAGER_LINUX_H

#include <QMutex>
#include <QNetworkInformation>

#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"

class NetworkDetectionManager_linux : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_linux(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_linux() override;
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface) override;
    bool isOnline() override;

private slots:
    void onReachabilityChanged(QNetworkInformation::Reachability newReachability);

private:
    bool isOnline_;
    types::NetworkInterface networkInterface_;

    void updateNetworkInfo(bool bWithEmitSignal);
    QString getDefaultRouteInterface(bool &isOnline);
    void getInterfacePars(const QString &ifname, types::NetworkInterface &outNetworkInterface);
    QString getMacAddressByIfName(const QString &ifname);
    bool isActiveByIfName(const QString &ifname);
    bool checkWirelessByIfName(const QString &ifname);
    QString getFriendlyNameByIfName(const QString &ifname);
};

#endif // NETWORKDETECTIONMANAGER_LINUX_H
