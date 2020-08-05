#ifndef OPENVPNCONNECTION_H
#define OPENVPNCONNECTION_H

#include <QThread>
#include <QElapsedTimer>
#include <QTimer>
#include <QMutex>
#include "engine/helper/ihelper.h"
#include "iconnection.h"
#include "engine/proxy/proxysettings.h"
#include "utils/boost_includes.h"
#include <atomic>

class OpenVPNConnection : public IConnection
{
    Q_OBJECT

public:
    explicit OpenVPNConnection(QObject *parent, IHelper *helper);
    ~OpenVPNConnection() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const ProxySettings &proxySettings,
                      bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() override;

    QString getConnectedTapTunAdapterName() override;

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;

protected:
    void run() override;

private slots:
    void onKillControllerTimer();

private:
    static constexpr int DEFAULT_PORT = 9544;
    static constexpr int MAX_WAIT_OPENVPN_ON_START = 20000;

    std::atomic<bool> bStopThread_;

    boost::asio::io_service io_service_;

    QString configPath_;
    QString username_;
    QString password_;
    ProxySettings proxySettings_;

    enum CONNECTION_STATUS {STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED_TO_SOCKET, STATUS_CONNECTED};
    CONNECTION_STATUS currentState_;
    mutable QMutex mutexCurrentState_;

    void setCurrentState(CONNECTION_STATUS state);
    void setCurrentStateAndEmitDisconnected(CONNECTION_STATUS state);
    void setCurrentStateAndEmitError(CONNECTION_STATUS state, CONNECTION_ERROR err);
    CONNECTION_STATUS getCurrentState() const;
    bool runOpenVPN(unsigned int port, const ProxySettings &proxySettings, unsigned long &outCmdId);

    struct StateVariables
    {
        boost::scoped_ptr<boost::asio::ip::tcp::socket> socket;
        boost::scoped_ptr<boost::asio::streambuf> buffer;
        bool bTapErrorEmited;
        bool bWasStateNotification;
        bool bWasSecondAttemptToStartOpenVpn;
        bool bWasSocketConnected;

        bool isAcceptSigTermCommand_;
        bool bSigTermSent;
        bool bNeedSendSigTerm;

        bool bFirstCalcStat;
        quint64 prevBytesRcved;
        quint64 prevBytesXmited;

        unsigned long lastCmdId;
        unsigned int openVpnPort;

        QElapsedTimer elapsedTimer;

        StateVariables()
        {
            reset();
        }

        void reset()
        {
            socket.reset();
            buffer.reset();

            bSigTermSent = false;
            bTapErrorEmited = false;
            bWasStateNotification = false;
            bWasSecondAttemptToStartOpenVpn = false;
            bFirstCalcStat = true;
            prevBytesRcved = 0;
            prevBytesXmited = 0;
            lastCmdId = 0;
            openVpnPort = 0;
            bWasSocketConnected = false;
            bNeedSendSigTerm = false;
            isAcceptSigTermCommand_ = false;
        }
    };

    StateVariables stateVariables_;
    QString tapAdapter_;
    QMutex tapAdapterMutex_;

    static constexpr int KILL_TIMEOUT = 10000;
    QTimer killControllerTimer_;

    void funcRunOpenVPN();
    void funcConnectToOpenVPN(const boost::system::error_code& err);
    void handleRead(const boost::system::error_code& err, size_t bytes_transferred);
    void funcDisconnect();
    QString safeGetTapAdapter();
    void safeSetTapAdapter(const QString &tapAdapter);

    void checkErrorAndContinue(boost::system::error_code &write_error, bool bWithAsyncReadCall);
    void continueWithUsernameImpl();
    void continueWithPasswordImpl();
};

#endif // OPENVPNCONNECTION_H
