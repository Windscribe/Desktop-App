#ifndef HELPER_LINUX_H
#define HELPER_LINUX_H

#include <QElapsedTimer>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include "ihelper.h"
#include "utils/boost_includes.h"
#include "../linux/helper/ipc/helper_commands.h"

class Helper_linux : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_linux(QObject *parent = 0);
    ~Helper_linux() override;

    void startInstallHelper() override;
    bool isHelperConnected() override;
    bool isFailedConnectToHelper() override;
    void setNeedFinish() override;
    QString getHelperVersion() override;
    bool reinstallHelper() override;

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) override;
    void clearUnblockingCmd(unsigned long cmdId) override;
    void suspendUnblockingCmd(unsigned long cmdId) override;

    bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts) override;
    void sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const ProtocolType &protocol) override;
    bool setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress) override;


    bool startWireGuard(const QString &exeName, const QString &deviceName) override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(WireGuardStatus *status) override;
    void setDefaultWireGuardDeviceName(const QString &deviceName) override;

protected:
    void run() override;

private:

};

#endif // HELPER_LINUX_H
