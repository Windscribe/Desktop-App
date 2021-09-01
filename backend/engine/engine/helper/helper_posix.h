#ifndef HELPER_POSIX_H
#define HELPER_POSIX_H

#include <QElapsedTimer>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include "ihelper.h"
#include "utils/boost_includes.h"
#include "../posix_common/helper_commands.h"

// common base helper for Linux/Mac
class Helper_posix : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_posix(QObject *parent = 0);
    ~Helper_posix() override;

    // Common functions
    //void startInstallHelper() override;
    bool isHelperConnected() override;
    bool isFailedConnectToHelper() override { return bFailedConnectToHelper_; };
    //bool reinstallHelper() override;
    void setNeedFinish() override;
    //QString getHelperVersion() override;

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) override;
    void clearUnblockingCmd(unsigned long cmdId) override;
    void suspendUnblockingCmd(unsigned long cmdId) override;

    bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts) override;
    void sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const ProtocolType &protocol) override;
    //bool setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress) override;

     // WireGuard functions
    bool startWireGuard(const QString &exeName, const QString &deviceName) override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(WireGuardStatus *status) override;
    void setDefaultWireGuardDeviceName(const QString &deviceName) override;

    // Posix specific functions
    QString executeRootCommand(const QString &commandLine, int *exitCode = nullptr);
    bool executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId);
    bool executeTaskKill(const QString &executableName);


protected:
    void run() override;

protected:

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
    int executeRootCommandImpl(const QString &commandLine, bool *bExecuted, QString &answer, int *exitCode = nullptr);
    //bool setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted);

    static void connectHandler(const boost::system::error_code &ec);
    void doDisconnectAndReconnect();

    bool readAnswer(CMD_ANSWER &outAnswer);
    bool sendCmdToHelper(int cmdId, const std::string &data);
};

#endif // HELPER_POSIX_H
