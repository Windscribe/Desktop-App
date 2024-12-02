#include <QStringRef>

#include "openvpnconnection.h"
#include "utils/ws_assert.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "utils/utils.h"
#include "types/enums.h"
#include "availableport.h"
#include "engine/openvpnversioncontroller.h"
#include "utils/ipvalidation.h"

#ifdef Q_OS_WIN
    #include "adapterutils_win.h"
    #include "engine/helper/helper_win.h"
    #include "types/global_consts.h"
    #include "utils/extraconfig.h"
#elif defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    #include "engine/helper/helper_posix.h"
#endif


OpenVPNConnection::OpenVPNConnection(QObject *parent, IHelper *helper) : IConnection(parent), helper_(helper),
    bStopThread_(false), currentState_(STATUS_DISCONNECTED),
    isAllowFirewallAfterCustomConfigConnection_(false), privKeyPassword_("")
{
    connect(&killControllerTimer_, &QTimer::timeout, this, &OpenVPNConnection::onKillControllerTimer);
}

OpenVPNConnection::~OpenVPNConnection()
{
    wait();
}

void OpenVPNConnection::startConnect(const QString &config, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password,
                                     const types::ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                                     bool isCustomConfig, const QString &overrideDnsIp)
{
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(wireGuardConfig);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isAutomaticConnectionMode);
    Q_UNUSED(overrideDnsIp);
    WS_ASSERT(getCurrentState() == STATUS_DISCONNECTED);

    qCDebug(LOG_CONNECTION) << "connectOVPN";

    bStopThread_ = true;
    wait();
    bStopThread_ = false;

    setCurrentState(STATUS_CONNECTING);
    config_ = config;
    username_ = username;
    password_ = password;
    proxySettings_ = proxySettings;
    isAllowFirewallAfterCustomConfigConnection_ = false;
    isCustomConfig_ = isCustomConfig;

    stateVariables_.reset();
    connectionAdapterInfo_.clear();
    start(LowPriority);
}

void OpenVPNConnection::startDisconnect()
{
    if (isDisconnected())
    {
        emit disconnected();
    }
    else
    {
        if (!killControllerTimer_.isActive())
        {
            killControllerTimer_.start(KILL_TIMEOUT);
        }

        bStopThread_ = true;
        io_service_.post(boost::bind( &OpenVPNConnection::funcDisconnect, this ));
    }
}

bool OpenVPNConnection::isDisconnected() const
{
    return getCurrentState() == STATUS_DISCONNECTED;
}

bool OpenVPNConnection::isAllowFirewallAfterCustomConfigConnection() const
{
    return isAllowFirewallAfterCustomConfigConnection_;
}

void OpenVPNConnection::continueWithUsernameAndPassword(const QString &username, const QString &password)
{
    username_ = username;
    password_ = password;
    io_service_.post(boost::bind( &OpenVPNConnection::continueWithUsernameImpl, this ));
}

void OpenVPNConnection::continueWithPassword(const QString &password)
{
    password_ = password;
    io_service_.post(boost::bind( &OpenVPNConnection::continueWithPasswordImpl, this ));
}

void OpenVPNConnection::continueWithPrivKeyPassword(const QString &password)
{
    privKeyPassword_ = password;
    io_service_.post(boost::bind( &OpenVPNConnection::continueWithPrivKeyPasswordImpl, this ));
}

void OpenVPNConnection::setCurrentState(CONNECTION_STATUS state)
{
    QMutexLocker locker(&mutexCurrentState_);
    currentState_ = state;
}

void OpenVPNConnection::setCurrentStateAndEmitDisconnected(OpenVPNConnection::CONNECTION_STATUS state)
{
    QMutexLocker locker(&mutexCurrentState_);
    // If we have already gotten to the disconnected state, do not send a duplicate signal
    if (currentState_ != STATUS_DISCONNECTED) {
        QTimer::singleShot(0, &killControllerTimer_, SLOT(stop()));
        currentState_ = state;
        emit disconnected();
    }
}

void OpenVPNConnection::setCurrentStateAndEmitError(OpenVPNConnection::CONNECTION_STATUS state, CONNECT_ERROR err)
{
    QMutexLocker locker(&mutexCurrentState_);
    currentState_ = state;
    emit error(err);
}

OpenVPNConnection::CONNECTION_STATUS OpenVPNConnection::getCurrentState() const
{
    QMutexLocker locker(&mutexCurrentState_);
    return currentState_;
}

IHelper::ExecuteError OpenVPNConnection::runOpenVPN(unsigned int port, const types::ProxySettings &proxySettings, unsigned long &outCmdId, bool isCustomConfig)
{
    QString httpProxy, socksProxy;
    unsigned int httpPort = 0, socksPort = 0;

    if (proxySettings.option() == PROXY_OPTION_HTTP) {
        httpProxy = proxySettings.address();
        httpPort = proxySettings.getPort();
    } else if (proxySettings.option() == PROXY_OPTION_SOCKS) {
        socksProxy = proxySettings.address();
        socksPort = proxySettings.getPort();
    } else if (proxySettings.option() == PROXY_OPTION_AUTODETECT) {
        WS_ASSERT(false);
    }

    qCInfo(LOG_CONNECTION) << "OpenVPN version:" << OpenVpnVersionController::instance().getOpenVpnVersion();

    return helper_->executeOpenVPN(config_, port, httpProxy, httpPort, socksProxy, socksPort, outCmdId, isCustomConfig);
}

void OpenVPNConnection::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
#ifdef Q_OS_WIN
    // NOTE: the emergency connect OpenVPN server is old-old and generates data packets not supported by the DCO driver.
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->createOpenVpnAdapter(!isCustomConfig_ && !isEmergencyConnect_ && ExtraConfig::instance().useOpenVpnDCO());
    helper_win->enableDnsLeaksProtection();
#endif

    io_service_.reset();
    io_service_.post(boost::bind( &OpenVPNConnection::funcRunOpenVPN, this ));
    io_service_.run();

#ifdef Q_OS_WIN
    helper_win->disableDnsLeaksProtection();
    helper_win->removeOpenVpnAdapter();
    // This prevents the adapter/network number from increasing on each connection.
    helper_win->removeWindscribeNetworkProfiles();
#endif
}

void OpenVPNConnection::onKillControllerTimer()
{
    qCInfo(LOG_CONNECTION) << "openvpn process not finished after " << KILL_TIMEOUT << "ms";
    qCInfo(LOG_CONNECTION) << "kill the openvpn process";
    killControllerTimer_.stop();
#ifdef Q_OS_WIN
    Helper_win *helper_win= dynamic_cast<Helper_win *>(helper_);
    helper_win->executeTaskKill(kTargetOpenVpn);
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetOpenVpn);
#endif

    setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
}

void OpenVPNConnection::funcRunOpenVPN()
{
    stateVariables_.openVpnPort = AvailablePort::getAvailablePort(DEFAULT_PORT);

    stateVariables_.elapsedTimer.start();

    int retries = 0;

    // run openvpn process
    while(runOpenVPN(stateVariables_.openVpnPort, proxySettings_, stateVariables_.lastCmdId, isCustomConfig_) != IHelper::EXECUTE_SUCCESS)
    {
        qCDebug(LOG_CONNECTION) << "Can't run OpenVPN";

        if (retries >= 2)
        {
            qCCritical(LOG_CONNECTION) << "Can't run openvpn process";
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
            return;
        }
        if (bStopThread_)
        {
            setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
            return;
        }
        retries++;

        msleep(1000);
    }

    qCInfo(LOG_CONNECTION) << "openvpn process runned: " << stateVariables_.openVpnPort;

    boost::asio::ip::tcp::endpoint endpoint;
    endpoint.port(stateVariables_.openVpnPort);
    endpoint.address(boost::asio::ip::address_v4::from_string("127.0.0.1"));
    stateVariables_.socket.reset(new boost::asio::ip::tcp::socket(io_service_));
    stateVariables_.socket->async_connect(endpoint, boost::bind(&OpenVPNConnection::funcConnectToOpenVPN, this,
                                                                boost::asio::placeholders::error));
}

void OpenVPNConnection::funcConnectToOpenVPN(const boost::system::error_code& err)
{
    if (err.value() == 0)
    {
        qCInfo(LOG_CONNECTION) << "Program connected to openvpn socket";
        helper_->suspendUnblockingCmd(stateVariables_.lastCmdId);
        setCurrentState(STATUS_CONNECTED_TO_SOCKET);
        stateVariables_.buffer.reset(new boost::asio::streambuf());
        boost::asio::async_read_until(*stateVariables_.socket, *stateVariables_.buffer, "\n",
                boost::bind(&OpenVPNConnection::handleRead, this,
                  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

        if (bStopThread_)
        {
            funcDisconnect();
        }
    }
    else
    {
        stateVariables_.socket.reset();

        // check timeout
        if (stateVariables_.elapsedTimer.elapsed() > MAX_WAIT_OPENVPN_ON_START)
        {
            qCCritical(LOG_CONNECTION) << "Can't connect to openvpn socket during"
                                    << (MAX_WAIT_OPENVPN_ON_START/1000) << "secs";
            helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, CONNECT_ERROR::NO_OPENVPN_SOCKET);
            return;
        }

        // check if openvpn process already finished
        QString logStr;
        bool bFinished;
        helper_->getUnblockingCmdStatus(stateVariables_.lastCmdId, logStr, bFinished);

        if (bFinished)
        {
            qCInfo(LOG_CONNECTION) << "openvpn process finished before connected to openvpn socket";
            qCInfo(LOG_CONNECTION) << "answer from openvpn process, answer =" << logStr;

            if (bStopThread_)
            {
                setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
                return;
            }

            //try second attempt to run openvpn after pause 2 sec
            if (!stateVariables_.bWasSecondAttemptToStartOpenVpn)
            {
                qCInfo(LOG_CONNECTION) << "try second attempt to run openvpn after pause 2 sec";
                msleep(2000);
                stateVariables_.bWasSecondAttemptToStartOpenVpn = true;
                io_service_.post(boost::bind( &OpenVPNConnection::funcRunOpenVPN, this ));
                return;
            }
            else
            {
                setCurrentStateAndEmitError(STATUS_DISCONNECTED, CONNECT_ERROR::NO_OPENVPN_SOCKET);
                return;
            }
        }

        boost::asio::ip::tcp::endpoint endpoint;
        endpoint.port(stateVariables_.openVpnPort);
        endpoint.address(boost::asio::ip::address_v4::from_string("127.0.0.1"));
        stateVariables_.socket.reset(new boost::asio::ip::tcp::socket(io_service_));
        stateVariables_.socket->async_connect(endpoint, boost::bind(&OpenVPNConnection::funcConnectToOpenVPN, this,
                                                                    boost::asio::placeholders::error));
    }
}

void OpenVPNConnection::handleRead(const boost::system::error_code &err, size_t bytes_transferred)
{
    Q_UNUSED(bytes_transferred);
    if (err.value() == 0)
    {
        std::istream is(stateVariables_.buffer.get());
        std::string resultLine;
        std::getline(is, resultLine);

        QString serverReply = QString::fromStdString(resultLine).trimmed();

        boost::system::error_code write_error;
        // skip log out BYTECOUNT
        if (!serverReply.contains(">BYTECOUNT:", Qt::CaseInsensitive))
        {
            qCInfo(LOG_OPENVPN) << serverReply;
        }
        if (serverReply.contains("HOLD:Waiting for hold release", Qt::CaseInsensitive))
        {
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer("state on all\n"), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.startsWith("END") && stateVariables_.bWasStateNotification)
        {
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer("log on\n"), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("SUCCESS: real-time state notification set to ON", Qt::CaseInsensitive))
        {
            stateVariables_.bWasStateNotification = true;
            stateVariables_.isAcceptSigTermCommand_ = true;
        }
        else if (serverReply.contains("SUCCESS: real-time log notification set to ON", Qt::CaseInsensitive))
        {
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer("bytecount 1\n"), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("SUCCESS: bytecount interval changed", Qt::CaseInsensitive))
        {
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer("hold release\n"), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("PASSWORD:Need 'Auth' username/password", Qt::CaseInsensitive))
        {
            if (!username_.isEmpty())
            {
                char message[1024];
                snprintf(message, 1024, "username \"Auth\" %s\n", username_.toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else
            {
                emit requestUsername();
            }
        }
        else if (serverReply.contains("PASSWORD:Need 'Private Key' password", Qt::CaseInsensitive))
        {
            if (!privKeyPassword_.isEmpty())
            {
                char message[1024];
                snprintf(message, 1024, "password \"Private Key\" %s\n", privKeyPassword_.toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else
            {
                emit requestPrivKeyPassword();
            }
        }
        else if (serverReply.contains("PASSWORD:Need 'HTTP Proxy' username/password", Qt::CaseInsensitive))
        {
            char message[1024];
            snprintf(message, 1024, "username \"HTTP Proxy\" %s\n", proxySettings_.getUsername().toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'HTTP Proxy' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            char message[1024];
            snprintf(message, 1024, "password \"HTTP Proxy\" %s\n", proxySettings_.getPassword().toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'Auth' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            if (!password_.isEmpty())
            {
                // See Command Parsing paragraph in management-notes.txt file of openvpn sources.
                // There are escaping rules for the openvpn password command.
                char message[1024];
                QString escaped = password_;
                escaped.replace("\\", "\\\\");
                escaped.replace("\"", "\\\"");
                escaped.replace("\t", "\\t");
                snprintf(message, 1024, "password \"Auth\" \"%s\"\n", escaped.toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else
            {
                emit requestPassword();
            }
        }
        else if (serverReply.contains("PASSWORD:Verification Failed: 'Auth'", Qt::CaseInsensitive))
        {
            emit error(CONNECT_ERROR::AUTH_ERROR);
            if (!stateVariables_.bSigTermSent)
            {
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
                helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
                stateVariables_.bSigTermSent = true;
            }
        }
        else if (serverReply.contains("FATAL:Error: private key password verification failed", Qt::CaseInsensitive))
        {
            emit error(CONNECT_ERROR::PRIV_KEY_PASSWORD_ERROR);
            if (!stateVariables_.bSigTermSent)
            {
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
                helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
                stateVariables_.bSigTermSent = true;
            }
        }
        else if (serverReply.contains("There are no TAP-Windows", Qt::CaseInsensitive) && serverReply.contains("Wintun",Qt::CaseInsensitive) &&
                 serverReply.contains("adapters on this system.",Qt::CaseInsensitive))
        {
            if (!stateVariables_.bTapErrorEmited)
            {
                emit error(CONNECT_ERROR::NO_INSTALLED_TUN_TAP);
                stateVariables_.bTapErrorEmited = true;
                if (!stateVariables_.bSigTermSent)
                {
                    boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
                    helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
                    stateVariables_.bSigTermSent = true;
                }
            }
        }
        else if (serverReply.startsWith(">BYTECOUNT:", Qt::CaseInsensitive))
        {
            QStringList pars = serverReply.split(":");
            if (pars.count() > 1)
            {
                QStringList pars2 = pars[1].split(",");
                if (pars2.count() == 2)
                {
                    quint64 l1 = pars2[0].toULongLong();
                    quint64 l2 = pars2[1].toULongLong();
                    if (stateVariables_.bFirstCalcStat)
                    {
                        stateVariables_.prevBytesRcved = l1;
                        stateVariables_.prevBytesXmited = l2;
                        emit statisticsUpdated(stateVariables_.prevBytesRcved, stateVariables_.prevBytesXmited, false);
                        stateVariables_.bFirstCalcStat = false;
                    }
                    else
                    {
                        emit statisticsUpdated(l1 - stateVariables_.prevBytesRcved, l2 - stateVariables_.prevBytesXmited, false);
                        stateVariables_.prevBytesRcved = l1;
                        stateVariables_.prevBytesXmited = l2;
                    }
                }
            }
        }
        else if (serverReply.startsWith(">STATE:", Qt::CaseInsensitive))
        {
            if (serverReply.contains("CONNECTED,SUCCESS", Qt::CaseInsensitive))
            {
#ifdef Q_OS_WIN
                AdapterGatewayInfo windscribeAdapter = AdapterUtils_win::getConnectedAdapterInfo(QString::fromWCharArray(kOpenVPNAdapterIdentifier));
                if (!windscribeAdapter.isEmpty())
                {
                    if (connectionAdapterInfo_.adapterIp() != windscribeAdapter.adapterIp())
                    {
                        qCCritical(LOG_CONNECTION) << "Error: Adapter IP detected from openvpn log not equal to the adapter IP from AdapterUtils_win::getWindscribeConnectedAdapterInfo()";
                        WS_ASSERT(false);
                    }
                    connectionAdapterInfo_.setAdapterName(windscribeAdapter.adapterName());
                    connectionAdapterInfo_.setAdapterIp(windscribeAdapter.adapterIp());
                    connectionAdapterInfo_.setDnsServers(windscribeAdapter.dnsServers());
                    connectionAdapterInfo_.setIfIndex(windscribeAdapter.ifIndex());
                }
                else
                {
                    qCCritical(LOG_CONNECTION) << "Can't detect connected Windscribe adapter";
                }
#endif

                QString remoteIp;
                if (parseConnectedSuccessReply(serverReply, remoteIp))
                {
                    connectionAdapterInfo_.setRemoteIp(remoteIp);
                }
                else
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse CONNECTED,SUCCESS control message";
                }
                setCurrentState(STATUS_CONNECTED);
                emit connected(connectionAdapterInfo_);
            }
            else if (serverReply.contains("CONNECTED,ERROR", Qt::CaseInsensitive))
            {
                setCurrentState(STATUS_CONNECTED);
                emit error(CONNECT_ERROR::CONNECTED_ERROR);
            }
            else if (serverReply.contains("RECONNECTING", Qt::CaseInsensitive))
            {
                stateVariables_.isAcceptSigTermCommand_ = false;
                stateVariables_.bWasStateNotification = false;
                setCurrentState(STATUS_CONNECTED_TO_SOCKET);
                emit reconnecting();
            }
        }
        else if (serverReply.startsWith(">LOG:", Qt::CaseInsensitive))
        {
            bool bContainsUDPWord = serverReply.contains("UDP", Qt::CaseInsensitive);
            if (bContainsUDPWord && serverReply.contains("No buffer space available (WSAENOBUFS) (code=10055)", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("No Route to Host (WSAEHOSTUNREACH) (code=10065)", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("Can't assign requested address (code=49)", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("No buffer space available (code=55)", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::UDP_NO_BUFFER_SPACE);
            }
            else if (bContainsUDPWord && serverReply.contains("Network is down (code=50)", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::UDP_NETWORK_DOWN);
            }
            else if (serverReply.contains("write_wintun", Qt::CaseInsensitive) && serverReply.contains("head/tail value is over capacity", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::WINTUN_OVER_CAPACITY);
            }
            else if (serverReply.contains("TCP:", Qt::CaseInsensitive) && serverReply.contains("failed", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::TCP_ERROR);
            }
            else if (serverReply.contains("Initialization Sequence Completed With Errors", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS);
            }
#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
            else if (serverReply.contains("device", Qt::CaseInsensitive) && serverReply.contains("opened", Qt::CaseInsensitive))
            {
                QString deviceName;
                if (parseDeviceOpenedReply(serverReply, deviceName))
                {
                    connectionAdapterInfo_.setAdapterName(deviceName);
                }
            }
#endif
            else if (serverReply.contains("PUSH: Received control message:", Qt::CaseInsensitive))
            {
                bool isRedirectDefaultGateway = true;
                if (!parsePushReply(serverReply, connectionAdapterInfo_, isRedirectDefaultGateway))
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse PUSH Received control message";
                }

                if (isRedirectDefaultGateway)
                {
                    // We are going to set up the default gateway, so firewall is allowed after
                    // we have connected (unless the current custom config explicitly forbits this).
                    isAllowFirewallAfterCustomConfigConnection_ = true;
                }
            } else if (serverReply.contains("write UDP: Unknown error (code=10065)") || serverReply.contains("write UDP: Unknown error (code=10054)")) {
                // These errors indicate socket was closed or otherwise unavailable for writing.
                setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
            }
        }
        else if (serverReply.contains(">FATAL:All wintun adapters on this system are currently in use", Qt::CaseInsensitive))
        {
            emit error(CONNECT_ERROR::WINTUN_FATAL_ERROR);
        }

        checkErrorAndContinue(write_error, true);
    }
    else
    {
        qCInfo(LOG_CONNECTION) << "Read from openvpn socket connection failed, error:" << QString::fromStdString(err.message());
        setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
    }
}

void OpenVPNConnection::funcDisconnect()
{
    int curState = getCurrentState();
    if (!stateVariables_.bSigTermSent && (curState == STATUS_CONNECTED_TO_SOCKET || curState == STATUS_CONNECTED))
    {
        if (stateVariables_.isAcceptSigTermCommand_)
        {
            boost::system::error_code write_error;
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
            helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
            stateVariables_.bSigTermSent = true;
        }
        else
        {
            stateVariables_.bNeedSendSigTerm = true;
        }
    }
}

void OpenVPNConnection::checkErrorAndContinue(boost::system::error_code &write_error, bool bWithAsyncReadCall)
{
    if (write_error.value() != 0)
    {
        qCWarning(LOG_CONNECTION) << "Write to openvpn socket connection failed, error:" << QString::fromStdString(write_error.message());
        setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
    }
    else
    {
        if (bWithAsyncReadCall)
        {
            boost::asio::async_read_until(*stateVariables_.socket, *stateVariables_.buffer, "\n",
                boost::bind(&OpenVPNConnection::handleRead, this,
                  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    if (stateVariables_.bNeedSendSigTerm && stateVariables_.isAcceptSigTermCommand_ && !stateVariables_.bSigTermSent)
    {
        boost::system::error_code new_write_error;
        boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), new_write_error);
        helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
        stateVariables_.bSigTermSent = true;
    }
}

void OpenVPNConnection::continueWithUsernameImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "username \"Auth\" %s\n", username_.toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::continueWithPasswordImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "password \"Auth\" %s\n", password_.toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::continueWithPrivKeyPasswordImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "password \"Private Key\" %s\n", privKeyPassword_.toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::setPrivKeyPassword(const QString &password)
{
    privKeyPassword_ = password;
}

bool OpenVPNConnection::parsePushReply(const QString &reply, AdapterGatewayInfo &outConnectionAdapterInfo, bool &outRedirectDefaultGateway)
{
    QStringRef str(&reply);

    int firstQuote = str.indexOf('\'');
    int lastQuote = str.lastIndexOf('\'');
    if (firstQuote == -1 || lastQuote == -1)
    {
        return false;
    }

    const QStringRef str2 = str.mid(firstQuote + 1, lastQuote - firstQuote - 1);
    const QVector<QStringRef> values =  str2.split(',');

    outRedirectDefaultGateway = false;
    outConnectionAdapterInfo.clear();

    for (auto it : values)
    {
        if (it.contains("redirect-gateway def1", Qt::CaseInsensitive))
        {
            outRedirectDefaultGateway = true;
        }
        else if (it.contains("route-gateway", Qt::CaseInsensitive))
        {
            const QVector<QStringRef> v = it.split(' ');
            if (v.count() != 2)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse route-gateway message";
                return false;
            }
            else
            {
                const QString ipStr = v[1].toString();
                if (!IpValidation::isIpv4Address(ipStr))
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse route-gateway message (incorrect IPv4 address)";
                    return false;
                }
                else
                {
                    outConnectionAdapterInfo.setGateway(ipStr);
                }
            }
        }
        else if (it.contains("ifconfig", Qt::CaseInsensitive))
        {
            const QVector<QStringRef> v = it.split(' ');
            if (v.count() != 3)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse ifconfig message";
                return false;
            }
            else
            {
                const QString ipStr = v[1].toString();
                if (!IpValidation::isIpv4Address(ipStr))
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse ifconfig message (incorrect IPv4 address)";
                    return false;
                }
                else
                {
                    outConnectionAdapterInfo.setAdapterIp(ipStr);
                }
            }
        }
        else if (it.contains("dhcp-option", Qt::CaseInsensitive))
        {
            const QVector<QStringRef> v = it.split(' ');
            if (v.count() != 3)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse dhcp-option message";
                return false;
            }
            else
            {
                if (v[1] == "DNS")
                {
                    const QString ipStr = v[2].toString();
                    if (!IpValidation::isIpv4Address(ipStr))
                    {
                        qCCritical(LOG_CONNECTION) << "Can't parse dhcp-option DNS message (incorrect IPv4 address)";
                        return false;
                    }
                    else
                    {
                        outConnectionAdapterInfo.addDnsServer(ipStr);
                    }
                }
            }
        }
    }
    return true;
}

bool OpenVPNConnection::parseDeviceOpenedReply(const QString &reply, QString &outDeviceName)
{
    QStringRef str(&reply);
    const QVector<QStringRef> v = str.split(',');
    if (v.count() != 3)
    {
        qCCritical(LOG_CONNECTION) << "Can't parse opened device message";
        return false;
    }
    const QVector<QStringRef> v2 = v.last().split(' ');
    if (v2.count() != 4)
    {
        qCCritical(LOG_CONNECTION) << "Can't parse opened device message (divide into 4 strings)";
        return false;
    }
#ifdef Q_OS_MACOS
    outDeviceName = v2[3].toString();
#elif defined (Q_OS_LINUX)
    outDeviceName = v2[2].toString();
#endif

    WS_ASSERT(outDeviceName.contains("tun"));
    return !outDeviceName.isEmpty();
}

bool OpenVPNConnection::parseConnectedSuccessReply(const QString &reply, QString &outRemoteIp)
{
    QStringRef str(&reply);
    const QVector<QStringRef> v = str.split(',');
    if (v.count() != 8)
    {
        qCCritical(LOG_CONNECTION) << "Can't parse CONNECT SUCCESS message (inccorect number of words)";
        return false;
    }
    else
    {
        outRemoteIp = v[4].toString();
        if (outRemoteIp.isEmpty())
        {
            qCCritical(LOG_CONNECTION) << "Can't parse CONNECT SUCCESS message (remote ip is empty)";
            return false;
        }
    }
    return true;
}

void OpenVPNConnection::setIsEmergencyConnect(bool isEmergencyConnect)
{
    isEmergencyConnect_ = isEmergencyConnect;
}
