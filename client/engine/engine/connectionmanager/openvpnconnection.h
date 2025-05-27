#pragma once

#include <QThread>
#include <QElapsedTimer>
#include <QTimer>
#include <QMutex>
#include "engine/helper/helper.h"
#include "iconnection.h"
#include "types/proxysettings.h"
#include "utils/boost_includes.h"
#include <atomic>

class OpenVPNConnection : public IConnection
{
    Q_OBJECT

public:
    explicit OpenVPNConnection(QObject *parent, Helper *helper);
    ~OpenVPNConnection() override;

    void startConnect(const QString &configOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                      bool isCustomConfig, const QString &overrideDnsIp) override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    bool isAllowFirewallAfterCustomConfigConnection() const override;

    ConnectionType getConnectionType() const override { return ConnectionType::OPENVPN; }

    void setPrivKeyPassword(const QString &password);
    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;
    void continueWithPrivKeyPassword(const QString &password);

    void setIsEmergencyConnect(bool isEmergencyConnect);

signals:
    void requestPrivKeyPassword();

protected:
    void run() override;

private slots:
    void onKillControllerTimer();

private:
    static constexpr int DEFAULT_PORT = 9544;
    static constexpr int MAX_WAIT_OPENVPN_ON_START = 20000;

    Helper *helper_;
    std::atomic<bool> bStopThread_;

    boost::asio::io_context io_context_;

    QString config_;
    QString username_;
    QString password_;
    QString privKeyPassword_;
    types::ProxySettings proxySettings_;
    bool isCustomConfig_;
    bool isEmergencyConnect_ = false;

    enum CONNECTION_STATUS {STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED_TO_SOCKET, STATUS_CONNECTED};
    CONNECTION_STATUS currentState_;
    mutable QMutex mutexCurrentState_;

    void setCurrentState(CONNECTION_STATUS state);
    void setCurrentStateAndEmitDisconnected(CONNECTION_STATUS state);
    void setCurrentStateAndEmitError(CONNECTION_STATUS state, CONNECT_ERROR err);
    CONNECTION_STATUS getCurrentState() const;
    bool runOpenVPN(unsigned int port, const types::ProxySettings &proxySettings, unsigned long &outCmdId, bool isCustomConfig);

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
    bool isAllowFirewallAfterCustomConfigConnection_;

    static constexpr int KILL_TIMEOUT = 10000;
    QTimer killControllerTimer_;

    AdapterGatewayInfo connectionAdapterInfo_;

    void funcRunOpenVPN();
    void funcConnectToOpenVPN(const boost::system::error_code& err);
    void handleRead(const boost::system::error_code& err, size_t bytes_transferred);
    void funcDisconnect();

    void checkErrorAndContinue(boost::system::error_code &write_error, bool bWithAsyncReadCall);
    void continueWithUsernameImpl();
    void continueWithPasswordImpl();
    void continueWithPrivKeyPasswordImpl();

    bool parsePushReply(const QString &reply, AdapterGatewayInfo &outConnectionAdapterInfo, bool &outRedirectDefaultGateway);
    bool parseDeviceOpenedReply(const QString &reply, QString &outDeviceName);
    bool parseConnectedSuccessReply(const QString &reply, QString &outRemoteIp);
};
