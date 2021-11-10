#ifndef NETWORKDETECTIONMANAGER_LINUX_H
#define NETWORKDETECTIONMANAGER_LINUX_H

#include <QMutex>
#include <QNetworkConfigurationManager>
#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"

class NetworkDetectionManager_linux : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_linux(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_linux() override;
    void updateCurrentNetworkInterface() override;
    bool isOnline() override;

private slots:
    void onNetworkUpdated();

private:
    bool isOnline_;
    ProtoTypes::NetworkInterface networkInterface_;
    QNetworkConfigurationManager *ncm_;


    QString getDefaultRouteInterface(bool &isOnline);
    void getInterfacePars(const QString &ifname, ProtoTypes::NetworkInterface &outNetworkInterface);
    QString getMacAddressByIfName(const QString &ifname);
    bool isActiveByIfName(const QString &ifname);
    bool checkWirelessByIfName(const QString &ifname);
    QString getFriendlyNameByIfName(const QString &ifname);
};

#endif // NETWORKDETECTIONMANAGER_LINUX_H
