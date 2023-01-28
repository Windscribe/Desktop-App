#include "openvpnconnection.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/types/types.h"
#include "availableport.h"
#include "engine/openvpnversioncontroller.h"
#include "utils/ipvalidation.h"

#include <QStringRef>

#ifdef Q_OS_WIN
    #include "adapterutils_win.h"
    #include "engine/helper/helper_win.h"
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    #include "engine/helper/helper_posix.h"
#endif

OpenVPNConnection::OpenVPNConnection(QObject *parent, IHelper *helper) : IConnection(parent), helper_(helper),
    bStopThread_(false), currentState_(STATUS_DISCONNECTED),
    isAllowFirewallAfterCustomConfigConnection_(false)
{
    connect(&killControllerTimer_, SIGNAL(timeout()), SLOT(onKillControllerTimer()));
}

OpenVPNConnection::~OpenVPNConnection()
{
    //disconnect();
    wait();
}

void OpenVPNConnection::startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName, const QString &username, const QString &password,
                                     const ProxySettings &proxySettings, const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(wireGuardConfig);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isAutomaticConnectionMode);
    Q_ASSERT(getCurrentState() == STATUS_DISCONNECTED);

    qCDebug(LOG_CONNECTION) << "connectOVPN";

    bStopThread_ = true;
    wait();
    bStopThread_ = false;

    setCurrentState(STATUS_CONNECTING);
    configPath_ = configPathOrUrl;
    username_ = username;
    password_ = password;
    proxySettings_ = proxySettings;
    isAllowFirewallAfterCustomConfigConnection_ = false;

    stateVariables_.reset();
    connectionAdapterInfo_.clear();
    start(LowPriority);
}

void OpenVPNConnection::startDisconnect()
{
    if (isDisconnected())
    {
        Q_EMIT disconnected();
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

void OpenVPNConnection::setCurrentState(CONNECTION_STATUS state)
{
    QMutexLocker locker(&mutexCurrentState_);
    currentState_ = state;
}

void OpenVPNConnection::setCurrentStateAndEmitDisconnected(OpenVPNConnection::CONNECTION_STATUS state)
{
    QMutexLocker locker(&mutexCurrentState_);
    QTimer::singleShot(0, &killControllerTimer_, SLOT(stop()));
    currentState_ = state;
    Q_EMIT disconnected();
}

void OpenVPNConnection::setCurrentStateAndEmitError(OpenVPNConnection::CONNECTION_STATUS state, ProtoTypes::ConnectError err)
{
    QMutexLocker locker(&mutexCurrentState_);
    currentState_ = state;
    Q_EMIT error(err);
}

OpenVPNConnection::CONNECTION_STATUS OpenVPNConnection::getCurrentState() const
{
    QMutexLocker locker(&mutexCurrentState_);
    return currentState_;
}

IHelper::ExecuteError OpenVPNConnection::runOpenVPN(unsigned int port, const ProxySettings &proxySettings, unsigned long &outCmdId)
{
#ifdef Q_OS_WIN
    QString httpProxy, socksProxy;
    unsigned int httpPort = 0, socksPort = 0;

    if (proxySettings.option() == PROXY_OPTION_HTTP)
    {
        httpProxy = proxySettings.address();
        httpPort = proxySettings.getPort();
    }
    else if (proxySettings.option() == PROXY_OPTION_SOCKS)
    {
        socksProxy = proxySettings.address();
        socksPort = proxySettings.getPort();
    }
    else if (proxySettings.option() == PROXY_OPTION_AUTODETECT)
    {
        Q_ASSERT(false);
    }

    qCDebug(LOG_CONNECTION) << "OpenVPN version:" << OpenVpnVersionController::instance().getSelectedOpenVpnVersion();

    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    return helper_win->executeOpenVPN(configPath_, port, httpProxy, httpPort, socksProxy, socksPort, outCmdId);

#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    QString strCommand = "--config \"" + configPath_ + "\" --management 127.0.0.1 " + QString::number(port) + " --management-query-passwords --management-hold --verb 3";
    if (proxySettings.option() == PROXY_OPTION_HTTP)
    {
        strCommand += " --http-proxy " + proxySettings.address() + " " + QString::number(proxySettings.getPort()) + " auto";
    }
    else if (proxySettings.option() == PROXY_OPTION_SOCKS)
    {
        strCommand += " --socks-proxy " + proxySettings.address() + " " + QString::number(proxySettings.getPort());
    }
    else if (proxySettings.option() == PROXY_OPTION_AUTODETECT)
    {
        Q_ASSERT(false);
    }
    qCDebug(LOG_CONNECTION) << "OpenVPN version:" << OpenVpnVersionController::instance().getSelectedOpenVpnVersion();
    //qCDebug(LOG_CONNECTION) << strCommand;

    std::wstring strOvpnConfigPath = Utils::getDirPathFromFullPath(configPath_.toStdWString());
    QString qstrOvpnConfigPath = QString::fromStdWString(strOvpnConfigPath);

    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    return helper_posix->executeOpenVPN(strCommand, qstrOvpnConfigPath, outCmdId);
#endif
}

void OpenVPNConnection::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    io_service_.reset();
    io_service_.post(boost::bind( &OpenVPNConnection::funcRunOpenVPN, this ));
    io_service_.run();
}

void OpenVPNConnection::onKillControllerTimer()
{
    qCDebug(LOG_CONNECTION) << "openvpn process not finished after " << KILL_TIMEOUT << "ms";
    qCDebug(LOG_CONNECTION) << "kill the openvpn process";
    killControllerTimer_.stop();
#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->executeTaskKill(OpenVpnVersionController::instance().getSelectedOpenVpnExecutable());
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeRootCommand("pkill -9 -f \"" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable() + "\"");
#endif
}

void OpenVPNConnection::funcRunOpenVPN()
{
    stateVariables_.openVpnPort = AvailablePort::getAvailablePort(DEFAULT_PORT);

    stateVariables_.elapsedTimer.start();

    int retries = 0;

    // run openvpn process
    IHelper::ExecuteError err;
    while((err = runOpenVPN(stateVariables_.openVpnPort, proxySettings_, stateVariables_.lastCmdId)) != IHelper::EXECUTE_SUCCESS)
    {
        qCDebug(LOG_CONNECTION) << "Can't run OpenVPN";

        // don't bother re-attempting if signature is invalid
        if (err == IHelper::EXECUTE_VERIFY_ERROR)
        {
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, ProtoTypes::ConnectError::EXE_VERIFY_OPENVPN_ERROR);
            return;
        }

        if (retries >= 2)
        {
            qCDebug(LOG_CONNECTION) << "Can't run openvpn process";
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, ProtoTypes::ConnectError::CANT_RUN_OPENVPN);
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

    qCDebug(LOG_CONNECTION) << "openvpn process runned: " << stateVariables_.openVpnPort;

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
        qCDebug(LOG_CONNECTION) << "Program connected to openvpn socket";
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
            qCDebug(LOG_CONNECTION) << "Can't connect to openvpn socket during"
                                    << (MAX_WAIT_OPENVPN_ON_START/1000) << "secs";
            helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, ProtoTypes::ConnectError::NO_OPENVPN_SOCKET);
            return;
        }

        // check if openvpn process already finished
        QString logStr;
        bool bFinished;
        helper_->getUnblockingCmdStatus(stateVariables_.lastCmdId, logStr, bFinished);

        if (bFinished)
        {
            qCDebug(LOG_CONNECTION) << "openvpn process finished before connected to openvpn socket";
            qCDebug(LOG_CONNECTION) << "answer from openvpn process, answer =" << logStr;

            if (bStopThread_)
            {
                setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
                return;
            }

            //try second attempt to run openvpn after pause 2 sec
            if (!stateVariables_.bWasSecondAttemptToStartOpenVpn)
            {
                qCDebug(LOG_CONNECTION) << "try second attempt to run openvpn after pause 2 sec";
                msleep(2000);
                stateVariables_.bWasSecondAttemptToStartOpenVpn = true;
                io_service_.post(boost::bind( &OpenVPNConnection::funcRunOpenVPN, this ));
                return;
            }
            else
            {
                setCurrentStateAndEmitError(STATUS_DISCONNECTED, ProtoTypes::ConnectError::NO_OPENVPN_SOCKET);
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
            qCDebug(LOG_OPENVPN) << serverReply;
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
                sprintf(message, "username \"Auth\" %s\n", username_.toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else
            {
                Q_EMIT requestUsername();
            }
        }
        else if (serverReply.contains("PASSWORD:Need 'HTTP Proxy' username/password", Qt::CaseInsensitive))
        {
            char message[1024];
            sprintf(message, "username \"HTTP Proxy\" %s\n", proxySettings_.getUsername().toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'HTTP Proxy' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            char message[1024];
            sprintf(message, "password \"HTTP Proxy\" %s\n", proxySettings_.getPassword().toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'Auth' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            if (!password_.isEmpty())
            {
                char message[1024];
                sprintf(message, "password \"Auth\" %s\n", password_.toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else
            {
                Q_EMIT requestPassword();
            }
        }
        else if (serverReply.contains("PASSWORD:Verification Failed: 'Auth'", Qt::CaseInsensitive))
        {
            Q_EMIT error(ProtoTypes::ConnectError::AUTH_ERROR);
            if (!stateVariables_.bSigTermSent)
            {
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
                helper_->clearUnblockingCmd(stateVariables_.lastCmdId);
                stateVariables_.bSigTermSent = true;
            }
        }
        else if (serverReply.contains("There are no TAP-Windows adapters on this system", Qt::CaseInsensitive))
        {
            if (!stateVariables_.bTapErrorEmited)
            {
                Q_EMIT error(ProtoTypes::ConnectError::NO_INSTALLED_TUN_TAP);
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
                        Q_EMIT statisticsUpdated(stateVariables_.prevBytesRcved, stateVariables_.prevBytesXmited, false);
                        stateVariables_.bFirstCalcStat = false;
                    }
                    else
                    {
                        Q_EMIT statisticsUpdated(l1 - stateVariables_.prevBytesRcved, l2 - stateVariables_.prevBytesXmited, false);
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
                AdapterGatewayInfo windscribeAdapter = AdapterUtils_win::getWindscribeConnectedAdapterInfo();
                if (!windscribeAdapter.isEmpty())
                {
                    if (connectionAdapterInfo_.adapterIp() != windscribeAdapter.adapterIp())
                    {
                        qCDebug(LOG_CONNECTION) << "Error: Adapter IP detected from openvpn log not equal to the adapter IP from AdapterUtils_win::getWindscribeConnectedAdapterInfo()";
                        Q_ASSERT(false);
                    }
                    connectionAdapterInfo_.setAdapterName(windscribeAdapter.adapterName());
                    connectionAdapterInfo_.setAdapterIp(windscribeAdapter.adapterIp());
                    connectionAdapterInfo_.setDnsServers(windscribeAdapter.dnsServers());
                    connectionAdapterInfo_.setIfIndex(windscribeAdapter.ifIndex());
                }
                else
                {
                    qCDebug(LOG_CONNECTION) << "Can't detect connected Windscribe adapter";
                }
#endif

                QString remoteIp;
                if (parseConnectedSuccessReply(serverReply, remoteIp))
                {
                    connectionAdapterInfo_.setRemoteIp(remoteIp);
                }
                else
                {
                    qCDebug(LOG_CONNECTION) << "Can't parse CONNECTED,SUCCESS control message";
                }
                setCurrentState(STATUS_CONNECTED);
                Q_EMIT connected(connectionAdapterInfo_);
            }
            else if (serverReply.contains("CONNECTED,ERROR", Qt::CaseInsensitive))
            {
                setCurrentState(STATUS_CONNECTED);
                Q_EMIT error(ProtoTypes::ConnectError::CONNECTED_ERROR);
            }
            else if (serverReply.contains("RECONNECTING", Qt::CaseInsensitive))
            {
                stateVariables_.isAcceptSigTermCommand_ = false;
                stateVariables_.bWasStateNotification = false;
                setCurrentState(STATUS_CONNECTED_TO_SOCKET);
                Q_EMIT reconnecting();
            }
        }
        else if (serverReply.startsWith(">LOG:", Qt::CaseInsensitive))
        {
            bool bContainsUDPWord = serverReply.contains("UDP", Qt::CaseInsensitive);
            if (bContainsUDPWord && serverReply.contains("No buffer space available (WSAENOBUFS) (code=10055)", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("No Route to Host (WSAEHOSTUNREACH) (code=10065)", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("Can't assign requested address (code=49)", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::UDP_CANT_ASSIGN);
            }
            else if (bContainsUDPWord && serverReply.contains("No buffer space available (code=55)", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::UDP_NO_BUFFER_SPACE);
            }
            else if (bContainsUDPWord && serverReply.contains("Network is down (code=50)", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::UDP_NETWORK_DOWN);
            }
            else if (serverReply.contains("write_wintun", Qt::CaseInsensitive) && serverReply.contains("head/tail value is over capacity", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::WINTUN_OVER_CAPACITY);
            }
            else if (serverReply.contains("TCP", Qt::CaseInsensitive) && serverReply.contains("failed", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::TCP_ERROR);
            }
            else if (serverReply.contains("Initialization Sequence Completed With Errors", Qt::CaseInsensitive))
            {
                Q_EMIT error(ProtoTypes::ConnectError::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS);
            }
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
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
                    qCDebug(LOG_CONNECTION) << "Can't parse PUSH Received control message";
                }

                if (isRedirectDefaultGateway)
                {
                    // We are going to set up the default gateway, so firewall is allowed after
                    // we have connected (unless the current custom config explicitly forbits this).
                    isAllowFirewallAfterCustomConfigConnection_ = true;
                }
            }
        }
        else if (serverReply.contains(">FATAL:All TAP-Windows adapters on this system are currently in use", Qt::CaseInsensitive))
        {
            Q_EMIT error(ProtoTypes::ConnectError::ALL_TAP_IN_USE);
        }

        checkErrorAndContinue(write_error, true);
    }
    else
    {
        qCDebug(LOG_CONNECTION) << "Read from openvpn socket connection failed, error:" << QString::fromStdString(err.message());
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
        qCDebug(LOG_CONNECTION) << "Write to openvpn socket connection failed, error:" << QString::fromStdString(write_error.message());
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
    sprintf(message, "username \"Auth\" %s\n", username_.toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::continueWithPasswordImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    sprintf(message, "password \"Auth\" %s\n", password_.toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
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
                qCDebug(LOG_CONNECTION) << "Can't parse route-gateway message";
                return false;
            }
            else
            {
                const QString ipStr = v[1].toString();
                if (!IpValidation::instance().isIp(ipStr))
                {
                    qCDebug(LOG_CONNECTION) << "Can't parse route-gateway message (incorrect IPv4 address)";
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
                qCDebug(LOG_CONNECTION) << "Can't parse ifconfig message";
                return false;
            }
            else
            {
                const QString ipStr = v[1].toString();
                if (!IpValidation::instance().isIp(ipStr))
                {
                    qCDebug(LOG_CONNECTION) << "Can't parse ifconfig message (incorrect IPv4 address)";
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
                qCDebug(LOG_CONNECTION) << "Can't parse dhcp-option message";
                return false;
            }
            else
            {
                if (v[1] == "DNS")
                {
                    const QString ipStr = v[2].toString();
                    if (!IpValidation::instance().isIp(ipStr))
                    {
                        qCDebug(LOG_CONNECTION) << "Can't parse dhcp-option DNS message (incorrect IPv4 address)";
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
        qCDebug(LOG_CONNECTION) << "Can't parse opened device message";
        return false;
    }
    const QVector<QStringRef> v2 = v.last().split(' ');
    if (v2.count() != 4)
    {
        qCDebug(LOG_CONNECTION) << "Can't parse opened device message (divide into 4 strings)";
        return false;
    }
#ifdef Q_OS_MAC
    outDeviceName = v2[3].toString();
#elif defined (Q_OS_LINUX)
    outDeviceName = v2[2].toString();
#endif

    Q_ASSERT(outDeviceName.contains("tun"));
    return !outDeviceName.isEmpty();
}

bool OpenVPNConnection::parseConnectedSuccessReply(const QString &reply, QString &outRemoteIp)
{
    QStringRef str(&reply);
    const QVector<QStringRef> v = str.split(',');
    if (v.count() != 8)
    {
        qCDebug(LOG_CONNECTION) << "Can't parse CONNECT SUCCESS message (inccorect number of words)";
        return false;
    }
    else
    {
        outRemoteIp = v[4].toString();
        if (outRemoteIp.isEmpty())
        {
            qCDebug(LOG_CONNECTION) << "Can't parse CONNECT SUCCESS message (remote ip is empty)";
            return false;
        }
    }
    return true;
}
