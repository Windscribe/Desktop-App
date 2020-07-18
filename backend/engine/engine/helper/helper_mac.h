#ifndef HELPER_MAC_H
#define HELPER_MAC_H

#include <QElapsedTimer>
#include <QThread>
#include <QWaitCondition>
#include "ihelper.h"
#include "Utils/boost_includes.h"
#include "../Mac/Helper/src/IPC/HelperCommands.h"

class Helper_mac : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_mac(QObject *parent = 0);
    virtual ~Helper_mac();

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
    virtual bool isFailedConnectToHelper() { return bFailedConnectToHelper_; }

    virtual void setNeedFinish()
    {
        bNeedFinish_ = true;
    }

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
    virtual void enableDnsLeaksProtection();
    virtual void disableDnsLeaksProtection();
    virtual bool reinstallWanIkev2();
    virtual bool enableWanIkev2();

    virtual QStringList getActiveNetworkInterfaces_mac();
    virtual bool setKeychainUsernamePassword(const QString &username, const QString &password);

    virtual bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value);
    virtual bool removeMacAddressRegistryProperty(QString subkeyInterfaceName) ;
    virtual bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp) ;
    virtual bool addIKEv2DefaultRoute();
    virtual bool removeWindscribeNetworkProfiles();

    virtual bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts);
    virtual void sendConnectStatus(bool isConnected, const SplitTunnelingNetworkInfo &stni);
    virtual bool setKextPath(const QString &kextPath);

protected:
    virtual void run();

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

        WAITING_DATA()
        {
            bWaitForBlockingCommand_ = true;
        }

    } WAITING_DATA;

    enum { MAX_WAIT_HELPER = 5000 };

    QString interfaceToSkip_;
    bool bIPV6State_;

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
