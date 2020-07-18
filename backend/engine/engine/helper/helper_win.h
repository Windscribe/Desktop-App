#ifndef HELPER_WIN_H
#define HELPER_WIN_H

#include "ihelper.h"
#include <QTimer>
#include <QMutex>
#include <atomic>
#include <Windows.h>
#include "../../../windows/windscribe_service/ipc/servicecommunication.h"
#include "../../../windows/windscribe_service/ipc/serialize_structs.h"

class Helper_win : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_win(QObject *parent = NULL);
    virtual ~Helper_win();

    virtual void startInstallHelper();
    virtual QString executeRootCommand(const QString &commandLine);
    virtual bool executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName);
    virtual bool executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId);
    virtual bool executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort,
                                const QString &socksProxy, unsigned int socksPort,
                                unsigned long &outCmdId);

    virtual bool executeTaskKill(const QString &executableName);
    virtual bool executeResetTap(const QString &tapName);
    virtual QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber);
    virtual QString executeWmicEnable(const QString &adapterName);
    virtual QString executeWmicGetConfigManagerErrorCode(const QString &adapterName);
    virtual bool executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid,
                                     unsigned long &outCmdId, const QString &eventName);
    virtual bool executeChangeMtu(const QString &adapter, int mtu);

    virtual bool clearDnsOnTap();
    virtual QString enableBFE();

    virtual void setIPv6EnabledInFirewall(bool b);
    virtual void setIPv6EnabledInOS(bool b);
    virtual bool IPv6StateInOS();

    virtual bool isHelperConnected();
    virtual bool isFailedConnectToHelper();

    virtual void setNeedFinish();
    virtual QString getHelperVersion();

    virtual void enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress);
    virtual void enableFirewallOnBoot(bool bEnable);

    virtual bool removeWindscribeUrlsFromHosts();
    virtual bool addHosts(const QString &hosts);
    virtual bool removeHosts();

    virtual bool reinstallHelper();

    virtual void closeAllTcpConnections(bool isKeepLocalSockets);
    virtual QStringList getProcessesList();

    virtual bool whitelistPorts(const QString &ports);
    virtual bool deleteWhitelistPorts();

    virtual void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished);
    virtual void clearUnblockingCmd(unsigned long cmdId);

    virtual bool isSupportedICS();

    virtual QStringList getActiveNetworkInterfaces_mac();
    virtual bool setKeychainUsernamePassword(const QString &username, const QString &password);

    virtual void enableDnsLeaksProtection();
    virtual void disableDnsLeaksProtection();

    virtual bool reinstallWanIkev2();
    virtual bool enableWanIkev2();

    virtual bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value);
    virtual bool removeMacAddressRegistryProperty(QString subkeyInterfaceName);
    virtual bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp);

    virtual bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts);

    virtual bool addIKEv2DefaultRoute();
    virtual bool removeWindscribeNetworkProfiles();
    virtual void sendDefaultRoute(const QString &gatewayIp, const QString &interfaceName, const QString &interfaceIp);
    virtual void sendConnectStatus(bool isConnected, const SplitTunnelingNetworkInfo &stni);
    virtual bool setKextPath(const QString &kextPath);

protected:
    virtual void run();

private:
    enum {MAX_WAIT_TIME_FOR_HELPER = 30000};
    enum {MAX_WAIT_TIME_FOR_PIPE = 10000};
    enum {CHECK_UNBLOCKING_CMD_PERIOD = 2000};

    QString helperLabel_;
    std::atomic<bool> bFailedConnectToHelper_;
    std::atomic<bool> bHelperConnected_;
    std::atomic<bool> bStopThread_;

    bool bIPV6State_;
    QMutex mutex_;

    MessagePacketResult sendCmdToHelper(int cmdId, const std::string &data);
    bool disableIPv6();
    bool enableIPv6();

    bool disableIPv6InOS();
    bool enableIPv6InOS();


    int debugGetActiveUnblockingCmdCount();

    friend class FirewallController_win;
    // friend functions for windows firewall controller class
    bool firewallOn(const QString &ip, bool bAllowLanTraffic);
    bool firewallChange(const QString &ip, bool bAllowLanTraffic);
    bool firewallOff();
    bool firewallActualState();
    void initVariables();

    bool readAllFromPipe(HANDLE hPipe, char *buf, DWORD len);
    bool writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len);
};

#endif // HELPER_WIN_H
