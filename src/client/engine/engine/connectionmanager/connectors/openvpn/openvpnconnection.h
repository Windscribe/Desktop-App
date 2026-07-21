#pragma once

#include <QThread>
#include <QElapsedTimer>
#include <QTimer>

#include <atomic>

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/connectionmanager/connectors/openvpn/openvpnsessionparams.h"
#include "engine/connectionmanager/connectors/openvpn/stunnelmanager.h"
#include "engine/connectionmanager/connectors/openvpn/wstunnelmanager.h"
#include "engine/helper/helper.h"
#include "types/protocol.h"
#include "utils/boost_includes.h"

class OpenVPNConnection : public IConnection
{
    Q_OBJECT

public:
    explicit OpenVPNConnection(QObject *parent, Helper *helper, types::Protocol protocol, const OpenVpnSessionParams &sessionParams);
    ~OpenVPNConnection() override;

    void prepare(const CurrentConnectionDescr &descr, const AttemptEnvironment &env) override;
    void teardown() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    bool isAllowFirewallAfterCustomConfigConnection() const override;

    void continueWithUserInput(const UserInputResponse &response) override;

protected:
    void run() override;

private slots:
    void onKillControllerTimer();
    void onWrapperStarted();

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    friend class TestOpenVPNConnection;
#endif
    static constexpr int MAX_WAIT_OPENVPN_ON_START = 20000;
    static constexpr int MANAGEMENT_SOCKET_RETRY_DELAY_MS = 100;

    Helper *helper_;
    OpenVpnSessionParams sessionParams_;
    StunnelManager *stunnelManager_;
    WstunnelManager *wstunnelManager_;
    std::atomic<bool> bStopThread_;

    boost::asio::io_context io_context_;
    boost::asio::steady_timer connectRetryTimer_;

    QString config_;
    QString username_;
    QString password_;
    QString privKeyPassword_;
    bool isCustomConfig_;
    // Set once startConnect() runs; teardown() must not issue the mac route delete for a connector
    // that never dialed (the helper would be hit with another session's ip).
    bool isDialed_ = false;

    enum CONNECTION_STATUS {STATUS_DISCONNECTED, STATUS_CONNECTING, STATUS_CONNECTED_TO_SOCKET, STATUS_CONNECTED};
    std::atomic<CONNECTION_STATUS> currentState_;

    void setCurrentState(CONNECTION_STATUS state);
    void setCurrentStateAndEmitDisconnected(CONNECTION_STATUS state);
    void setCurrentStateAndEmitError(CONNECTION_STATUS state, CONNECT_ERROR err);
    CONNECTION_STATUS getCurrentState() const;
    bool runOpenVPN();

#ifdef Q_OS_WIN
    using ManagementSocket = boost::asio::ip::tcp::socket;
#else
    using ManagementSocket = boost::asio::local::stream_protocol::socket;
#endif

    struct StateVariables
    {
        boost::scoped_ptr<ManagementSocket> socket;
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

        unsigned int openVpnPort;
        unsigned long openVpnPid;

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
            openVpnPort = 0;
            openVpnPid = 0;
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
    void asyncConnectToManagementSocket();
    void retryConnectToManagementSocket(const boost::system::error_code& err);
#ifdef Q_OS_WIN
    bool verifyOpenVpnPeer();
#endif
    void funcConnectToOpenVPN(const boost::system::error_code& err);
    void handleRead(const boost::system::error_code& err, size_t bytes_transferred);
    void funcDisconnect();

    void checkErrorAndContinue(boost::system::error_code &write_error, bool bWithAsyncReadCall);
    void emitAuthErrorAndSigTerm(boost::system::error_code &write_error);
    void continueWithUsernameAndPassword(const QString &username, const QString &password);
    void continueWithPassword(const QString &password);
    void continueWithPrivKeyPassword(const QString &password);
    void continueWithUsernameImpl();
    void continueWithPasswordImpl();
    void continueWithPrivKeyPasswordImpl();

    static QString sanitizeString(const QString &s);

    bool parsePushReply(const QString &reply, AdapterGatewayInfo &outConnectionAdapterInfo, bool &outRedirectDefaultGateway);
    bool parseDeviceOpenedReply(const QString &reply, QString &outDeviceName);
    bool parseConnectedSuccessReply(const QString &reply, QString &outRemoteIp);
};
