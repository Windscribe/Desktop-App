#ifndef HELPER_MAC_H
#define HELPER_MAC_H

#include <QElapsedTimer>
#include <QThread>
#include <QWaitCondition>
#include "ihelper.h"
#include "utils/boost_includes.h"
#include "../mac/helper/src/ipc/helper_commands.h"

class Helper_mac : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_mac(QObject *parent = 0);
    ~Helper_mac() override;

    void startInstallHelper() override;
    QString executeRootCommand(const QString &commandLine) override;
    bool executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName) override;
    bool executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId) override;
    bool executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort,
                        const QString &socksProxy, unsigned int socksPort,
                        unsigned long &outCmdId) override;
    bool executeTaskKill(const QString &executableName) override;
    bool executeResetTap(const QString &tapName) override;
    QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber) override;
    QString executeWmicEnable(const QString &adapterName) override;
    QString executeWmicGetConfigManagerErrorCode(const QString &adapterName) override;
    bool executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid,
                          unsigned long &outCmdId, const QString &eventName) override;
    bool executeChangeMtu(const QString &adapter, int mtu) override;

    QString executeUpdateInstaller(const QString &installerPath, bool &success) override;

    bool clearDnsOnTap() override;
    QString enableBFE() override;
    QString resetAndStartRAS() override;

    void setIPv6EnabledInFirewall(bool b) override;
    void setIPv6EnabledInOS(bool b) override;
    bool IPv6StateInOS() override;


    bool isHelperConnected() override;
    bool isFailedConnectToHelper() override { return bFailedConnectToHelper_; }

    void setNeedFinish() override
    {
        bNeedFinish_ = true;
    }

    QString getHelperVersion() override;

    void enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress) override;

    void enableFirewallOnBoot(bool bEnable) override;
    bool removeWindscribeUrlsFromHosts() override;
    bool addHosts(const QString &hosts) override;
    bool removeHosts() override;

    bool reinstallHelper() override;

    void closeAllTcpConnections(bool isKeepLocalSockets) override;
    QStringList getProcessesList() override;

    bool whitelistPorts(const QString &ports) override;
    bool deleteWhitelistPorts() override;

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) override;
    void clearUnblockingCmd(unsigned long cmdId) override;
    void suspendUnblockingCmd(unsigned long cmdId) override;
    void enableDnsLeaksProtection() override;
    void disableDnsLeaksProtection() override;
    bool reinstallWanIkev2() override;
    bool enableWanIkev2() override;

    QStringList getActiveNetworkInterfaces_mac() override;
    bool setKeychainUsernamePassword(const QString &username, const QString &password) override;

    bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value) override;
    bool removeMacAddressRegistryProperty(QString subkeyInterfaceName) override;
    bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp) override;
    bool addIKEv2DefaultRoute() override;
    bool removeWindscribeNetworkProfiles() override;
    void setIKEv2IPSecParameters() override;

    bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts) override;
    void sendConnectStatus(bool isConnected, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const ProtocolType &protocol) override;

    bool setKextPath(const QString &kextPath) override;

    bool startWireGuard(const QString &exeName, const QString &deviceName) override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(WireGuardStatus *status) override;

protected:
    void run() override;

private:

    typedef struct WAITING_DATA
    {
        QWaitCondition waitCondition_;
        QMutex  mutexWaitCondition_;
        bool bWaitForBlockingCommand_;
        unsigned long waitingCmdId_;
        QString waitingAnswer_;
        int executed_;
        bool bReceivedAnswer_;

        WAITING_DATA() : bWaitForBlockingCommand_(true),  waitingCmdId_(0), executed_(0),
                         bReceivedAnswer_(false)
        {
        }

    } WAITING_DATA;

    enum { MAX_WAIT_HELPER = 5000 };

    QString interfaceToSkip_;
    bool bIPV6State_;

    QString wireGuardExeName_;
    QString wireGuardDeviceName_;

    // for blocking execute
    WAITING_DATA waitingData_;

    QMutex mutex_;
    unsigned long cmdId_;

    std::atomic<unsigned long> lastOpenVPNCmdId_;
    std::atomic<bool> bFailedConnectToHelper_;

    boost::asio::io_service io_service_;
    boost::asio::local::stream_protocol::endpoint ep_;
    boost::scoped_ptr<boost::asio::local::stream_protocol::socket> socket_;
    QMutex mutexSocket_;

    QElapsedTimer reconnectElapsedTimer_;
    bool bHelperConnectedEmitted_;

    bool bHelperConnected_;
    QMutex mutexHelperConnected_;
    void setHelperConnected(bool b)
    {
        QMutexLocker locker(&mutexHelperConnected_);
        bHelperConnected_ = b;
    }

    std::atomic<bool> bNeedFinish_;
    bool isNeedFinish()
    {
        return bNeedFinish_;
    }

    enum { RET_SUCCESS, RET_DISCONNECTED };
    int executeRootCommandImpl(const QString &commandLine, bool *bExecuted, QString &answer);
    bool setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted);

    static void connectHandler(const boost::system::error_code &ec);
    void doDisconnectAndReconnect();

    bool readAnswer(CMD_ANSWER &outAnswer);
    bool sendCmdToHelper(int cmdId, const std::string &data);
};

#endif // HELPER_MAC_H
