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
    void getCurrentNetworkInterface(ProtoTypes::NetworkInterface &networkInterface) override;
    bool isOnline() override;

private slots:
    void isMeteredChanged(bool _isMetered);
    void reachabilityChanged(QNetworkInformation::Reachability _newReachability);
    void transportMediumChanged(QNetworkInformation::TransportMedium _current);

private:
    bool isOnline_;
    ProtoTypes::NetworkInterface networkInterface_;
    QNetworkInformation *ncm_;


    void updateNetworkInfo(bool bWithEmitSignal);
    QString getDefaultRouteInterface(bool &isOnline);
    void getInterfacePars(const QString &ifname, ProtoTypes::NetworkInterface &outNetworkInterface);
    QString getMacAddressByIfName(const QString &ifname);
    bool isActiveByIfName(const QString &ifname);
    bool checkWirelessByIfName(const QString &ifname);
    QString getFriendlyNameByIfName(const QString &ifname);
};

#endif // NETWORKDETECTIONMANAGER_LINUX_H
