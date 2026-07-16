#include "openvpnconnection.h"

#include <chrono>

#include "types/enums.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/openvpnversioncontroller.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
    #include "engine/connectionmanager/adapterutils_win.h"
    #include "types/global_consts.h"
    #include "utils/extraconfig.h"
    #include "utils/winutils.h"
#elif defined (Q_OS_LINUX)
    #include <sys/socket.h>
    #include "engine/helper/helper_posix.h"
    #include "engine/helper/helper_linux.h"
    #include "utils/extraconfig.h"
#elif defined (Q_OS_MACOS)
    #include <sys/types.h>
    #include <unistd.h>
    #include "engine/helper/helper_posix.h"
#endif


OpenVPNConnection::OpenVPNConnection(QObject *parent, Helper *helper) : IConnection(parent), helper_(helper),
    bStopThread_(false), connectRetryTimer_(io_context_), currentState_(STATUS_DISCONNECTED),
    isAllowFirewallAfterCustomConfigConnection_(false), privKeyPassword_("")
{
    connect(&killControllerTimer_, &QTimer::timeout, this, &OpenVPNConnection::onKillControllerTimer);
}

OpenVPNConnection::~OpenVPNConnection()
{
    wait();
}

void OpenVPNConnection::startConnect(const StartConnectParams &params)
{
    const auto &p = std::get<OpenVpnStartParams>(params);
    WS_ASSERT(getCurrentState() == STATUS_DISCONNECTED);

    qCDebug(LOG_CONNECTION) << "connectOVPN";

    bStopThread_ = true;
    wait();
    bStopThread_ = false;

    setCurrentState(STATUS_CONNECTING);
    config_ = p.config;
    username_ = p.username;
    password_ = p.password;
    proxySettings_ = p.proxySettings;
    isAllowFirewallAfterCustomConfigConnection_ = false;
    isCustomConfig_ = p.isCustomConfig;

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
        boost::asio::post(io_context_, boost::bind( &OpenVPNConnection::funcDisconnect, this));
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
    boost::asio::post(io_context_, boost::bind( &OpenVPNConnection::continueWithUsernameImpl, this));
}

void OpenVPNConnection::continueWithPassword(const QString &password)
{
    password_ = password;
    boost::asio::post(io_context_, boost::bind( &OpenVPNConnection::continueWithPasswordImpl, this));
}

void OpenVPNConnection::continueWithPrivKeyPassword(const QString &password)
{
    privKeyPassword_ = password;
    boost::asio::post(io_context_, boost::bind( &OpenVPNConnection::continueWithPrivKeyPasswordImpl, this));
}

void OpenVPNConnection::setCurrentState(CONNECTION_STATUS state)
{
    currentState_ = state;
}

void OpenVPNConnection::setCurrentStateAndEmitDisconnected(OpenVPNConnection::CONNECTION_STATUS state)
{
    if (currentState_.exchange(state) != STATUS_DISCONNECTED) {
        QTimer::singleShot(0, &killControllerTimer_, SLOT(stop()));
        emit disconnected();
    }
}

void OpenVPNConnection::setCurrentStateAndEmitError(OpenVPNConnection::CONNECTION_STATUS state, CONNECT_ERROR err)
{
    currentState_ = state;
    emit error(err);
}

OpenVPNConnection::CONNECTION_STATUS OpenVPNConnection::getCurrentState() const
{
    return currentState_.load();
}

bool OpenVPNConnection::runOpenVPN()
{
    QString httpProxy, socksProxy;
    unsigned int httpPort = 0, socksPort = 0;

    // OpenVPN will only allow use of the proxy settings if the protocol is TCP.  Only send them to the helper
    // if they can be used, to avoid it inadvertently applying them to an unsupported protocol.
    if (protocol_ == types::Protocol::OPENVPN_TCP) {
        if (proxySettings_.option() == PROXY_OPTION_HTTP) {
            httpProxy = proxySettings_.address();
            httpPort = proxySettings_.getPort();
        } else if (proxySettings_.option() == PROXY_OPTION_SOCKS) {
            socksProxy = proxySettings_.address();
            socksPort = proxySettings_.getPort();
        } else if (proxySettings_.option() == PROXY_OPTION_AUTODETECT) {
            WS_ASSERT(false);
        }
    }

    qCInfo(LOG_CONNECTION) << "OpenVPN version:" << OpenVpnVersionController::instance().getOpenVpnVersion();

#ifdef Q_OS_WIN
    // OpenVPN selects its own management port; the helper reports the resolved port and the OpenVPN
    // PID so we can verify the management peer once connected.
    unsigned int port = 0;
    unsigned long pid = 0;
    const bool ok = helper_->executeOpenVPN(config_, httpProxy, httpPort, socksProxy, socksPort, port, pid);
    if (ok) {
        stateVariables_.openVpnPort = port;
        stateVariables_.openVpnPid = pid;
    }
    return ok;
#else
    // On POSIX the management interface is a root-owned unix domain socket (fixed path); no port.
    return helper_->executeOpenVPN(config_, httpProxy, httpPort, socksProxy, socksPort);
#endif
}

void OpenVPNConnection::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
#ifdef Q_OS_WIN
    // NOTE: The emergency connect OpenVPN server is old-old and generates data packets not supported by the DCO driver.
    const bool useDCODriver = !isCustomConfig_ && !isEmergencyConnect_ && ExtraConfig::instance().useOpenVpnDCO() && !(proxySettings_.isProxyEnabled() && protocol_ == types::Protocol::OPENVPN_TCP);
    helper_->createOpenVpnAdapter(useDCODriver);
    helper_->enableDnsLeaksProtection();
#elif defined (Q_OS_LINUX)
    // NOTE: The emergency connect OpenVPN server is old-old and generates data packets not supported by the DCO driver.
    const bool useDCODriver = !isEmergencyConnect_ && ExtraConfig::instance().useOpenVpnDCO() && !(proxySettings_.isProxyEnabled() && protocol_ == types::Protocol::OPENVPN_TCP);
    helper_->setOpenVpnDcoMode(useDCODriver);
#endif

    io_context_.restart();
    boost::asio::post(io_context_, boost::bind( &OpenVPNConnection::funcRunOpenVPN, this));
    io_context_.run();

#ifdef Q_OS_WIN
    helper_->disableDnsLeaksProtection();
    helper_->removeOpenVpnAdapter();
    // This prevents the adapter/network number from increasing on each connection.
    helper_->removeAppNetworkProfiles();
#endif
}

void OpenVPNConnection::onKillControllerTimer()
{
    qCInfo(LOG_CONNECTION) << "openvpn process not finished after " << KILL_TIMEOUT << "ms";
    qCInfo(LOG_CONNECTION) << "kill the openvpn process";
    killControllerTimer_.stop();
#ifdef Q_OS_WIN
    helper_->executeTaskKill(kTargetOpenVpn);
#else
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeTaskKill(kTargetOpenVpn);
#endif

    setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
}

void OpenVPNConnection::funcRunOpenVPN()
{
    stateVariables_.elapsedTimer.start();

    int retries = 0;

    // run openvpn process. On Windows this also populates stateVariables_.openVpnPort/openVpnPid
    // (OpenVPN picks its own management port and the helper reports it back).
    while(!runOpenVPN())
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

#ifdef Q_OS_WIN
    qCInfo(LOG_CONNECTION) << "ran openvpn process, management port:" << stateVariables_.openVpnPort;
#else
    // POSIX using domain sockets, so no port to report..
    qCInfo(LOG_CONNECTION) << "ran openvpn process";
#endif

    asyncConnectToManagementSocket();
}

void OpenVPNConnection::asyncConnectToManagementSocket()
{
#ifdef Q_OS_WIN
    boost::asio::ip::tcp::endpoint endpoint;
    endpoint.port(stateVariables_.openVpnPort);
    endpoint.address(boost::asio::ip::make_address("127.0.0.1"));
#else
    boost::asio::local::stream_protocol::endpoint endpoint(WS_OVPN_MGMT_SOCKET);
#endif
    stateVariables_.socket.reset(new ManagementSocket(io_context_));
    stateVariables_.socket->async_connect(endpoint, boost::bind(&OpenVPNConnection::funcConnectToOpenVPN, this,
                                                                boost::asio::placeholders::error));
}

#ifdef Q_OS_WIN
bool OpenVPNConnection::verifyOpenVpnPeer()
{
    if (!stateVariables_.socket) {
        return false;
    }

    boost::system::error_code ec;
    const auto localEp = stateVariables_.socket->local_endpoint(ec);
    if (ec) {
        qCCritical(LOG_CONNECTION) << "verifyOpenVpnPeer: local_endpoint failed:" << QString::fromStdString(ec.message());
        return false;
    }

    const quint16 localPort = localEp.port();                                          // engine end (E)
    const quint16 managementPort = static_cast<quint16>(stateVariables_.openVpnPort);  // OpenVPN end (P)

    // The OpenVPN-side connection entry has local port == management port (P) and remote port == our
    // local port (E). Its owning PID must match the PID the helper reported for the process it
    // spawned (whose binary the helper signature-verified before launch, and whose handle the helper
    // keeps open so the PID cannot be reused while OpenVPN runs).
    DWORD ownerPid = 0;
    if (!WinUtils::getTcpConnectionOwnerPid(managementPort, localPort, ownerPid)) {
        qCCritical(LOG_CONNECTION) << "verifyOpenVpnPeer: could not find owner PID for the management connection";
        return false;
    }

    if (stateVariables_.openVpnPid == 0 || ownerPid != stateVariables_.openVpnPid) {
        qCCritical(LOG_CONNECTION) << "verifyOpenVpnPeer: management peer PID mismatch, expected"
                                   << stateVariables_.openVpnPid << "got" << ownerPid;
        return false;
    }

    qCInfo(LOG_CONNECTION) << "OpenVPN management peer verified (pid" << ownerPid << ", port" << managementPort << ")";
    return true;
}
#endif

void OpenVPNConnection::funcConnectToOpenVPN(const boost::system::error_code& err)
{
    if (err.value() == 0) {
        // Before exchanging anything (credentials, control commands) over the management interface,
        // verify the peer is the genuine, helper-spawned OpenVPN. This defeats a local attacker that
        // grabbed/co-bound the management socket to impersonate OpenVPN and harvest credentials or
        // spoof connection state.
#ifdef Q_OS_WIN
        if (!verifyOpenVpnPeer())
        {
            qCCritical(LOG_CONNECTION) << "OpenVPN management peer verification failed; aborting connection";
            QTimer::singleShot(0, &killControllerTimer_, SLOT(stop()));
            stateVariables_.socket.reset();
            helper_->executeTaskKill(kTargetOpenVpn);
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, CONNECT_ERROR::NO_OPENVPN_SOCKET);
            return;
        }
#endif
        qCInfo(LOG_CONNECTION) << "Program connected to openvpn socket";
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
            setCurrentStateAndEmitError(STATUS_DISCONNECTED, CONNECT_ERROR::NO_OPENVPN_SOCKET);
            return;
        }

        // Back off before retrying. The connect can fail immediately (in particular a POSIX
        // unix-domain connect() while OpenVPN is still binding its socket), so re-arming with no
        // delay would busy-spin the io_context thread until the timeout above. Use an async timer
        // rather than a blocking sleep so the event loop stays responsive.
        connectRetryTimer_.expires_after(std::chrono::milliseconds(MANAGEMENT_SOCKET_RETRY_DELAY_MS));
        connectRetryTimer_.async_wait(boost::bind(&OpenVPNConnection::retryConnectToManagementSocket, this,
                                                  boost::asio::placeholders::error));
    }
}

void OpenVPNConnection::retryConnectToManagementSocket(const boost::system::error_code& err)
{
    // Timer cancelled (teardown) — do not re-arm the connect.
    if (err == boost::asio::error::operation_aborted) {
        return;
    }

    if (bStopThread_) {
        // Let the kill timer terminate the OpenVPN process we're unable to connect to.
        return;
    }

    asyncConnectToManagementSocket();
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
                snprintf(message, 1024, "username \"Auth\" \"%s\"\n", sanitizeString(username_).toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else if (isCustomConfig_)
            {
                emit requestUsername();
            }
            else
            {
                // Only custom configs prompt for credentials; for API/static IP locations a missing one is an auth failure.
                emitAuthErrorAndSigTerm(write_error);
            }
        }
        else if (serverReply.contains("PASSWORD:Need 'Private Key' password", Qt::CaseInsensitive))
        {
            if (!privKeyPassword_.isEmpty())
            {
                char message[1024];
                snprintf(message, 1024, "password \"Private Key\" \"%s\"\n", sanitizeString(privKeyPassword_).toUtf8().data());
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
            snprintf(message, 1024, "username \"HTTP Proxy\" \"%s\"\n", sanitizeString(proxySettings_.getUsername()).toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'HTTP Proxy' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            char message[1024];
            snprintf(message, 1024, "password \"HTTP Proxy\" \"%s\"\n", sanitizeString(proxySettings_.getPassword()).toUtf8().data());
            boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
        }
        else if (serverReply.contains("'Auth' username entered, but not yet verified", Qt::CaseInsensitive))
        {
            if (!password_.isEmpty())
            {
                // See Command Parsing paragraph in management-notes.txt file of openvpn sources.
                // There are escaping rules for the openvpn password command.
                char message[1024];
                snprintf(message, 1024, "password \"Auth\" \"%s\"\n", sanitizeString(password_).toUtf8().data());
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);
            }
            else if (isCustomConfig_)
            {
                emit requestPassword();
            }
            else
            {
                // Only custom configs prompt for credentials; for API/static IP locations a missing one is an auth failure.
                emitAuthErrorAndSigTerm(write_error);
            }
        }
        else if (serverReply.contains("PASSWORD:Verification Failed: 'Auth'", Qt::CaseInsensitive))
        {
            emitAuthErrorAndSigTerm(write_error);
        }
        else if (serverReply.contains("FATAL:Error: private key password verification failed", Qt::CaseInsensitive))
        {
            emit error(CONNECT_ERROR::PRIV_KEY_PASSWORD_ERROR);
            if (!stateVariables_.bSigTermSent)
            {
                boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
                stateVariables_.bSigTermSent = true;
            }
        }
        else if (serverReply.contains("There are no TAP-Windows", Qt::CaseInsensitive) && serverReply.contains("adapters on this system", Qt::CaseInsensitive))
        {
            if (!stateVariables_.bTapErrorEmited)
            {
                emit error(CONNECT_ERROR::NO_INSTALLED_TUN_TAP);
                stateVariables_.bTapErrorEmited = true;
                if (!stateVariables_.bSigTermSent)
                {
                    boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
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
                AdapterGatewayInfo vpnAdapter = AdapterUtils_win::getConnectedAdapterInfo(QString::fromWCharArray(kOpenVPNAdapterIdentifier));
                if (!vpnAdapter.isEmpty())
                {
                    if (connectionAdapterInfo_.adapterIpV4() != vpnAdapter.adapterIpV4())
                    {
                        qCCritical(LOG_CONNECTION) << "Error: Adapter IP detected from openvpn log not equal to the adapter IP from AdapterUtils_win::getConnectedAdapterInfo()";
                        WS_ASSERT(false);
                    }
                    connectionAdapterInfo_.setAdapterName(vpnAdapter.adapterName());
                    connectionAdapterInfo_.setDnsServers(vpnAdapter.dnsServers());
                    connectionAdapterInfo_.setIfIndex(vpnAdapter.ifIndex());
                }
                else
                {
                    qCCritical(LOG_CONNECTION) << "Can't detect connected VPN adapter";
                }
#endif

                QString remoteIp;
                if (parseConnectedSuccessReply(serverReply, remoteIp))
                {
                    connectionAdapterInfo_.setRemoteIp(types::IpAddress(remoteIp.toStdString()));
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
            else if (serverReply.contains("TCP:", Qt::CaseInsensitive) && serverReply.contains("failed", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::TCP_ERROR);
            }
            else if (serverReply.contains("connect to [AF_INET]127.0.0.1", Qt::CaseInsensitive) &&
                     serverReply.contains("Connection refused", Qt::CaseInsensitive))
            {
                qCInfo(LOG_CONNECTION) << "Localhost proxy (stunnel/wstunnel) connection refused, force-killing OpenVPN immediately";
                // We are on the io_context worker thread; marshal stop() to the timer's (engine) thread.
                QTimer::singleShot(0, &killControllerTimer_, SLOT(stop()));
                helper_->executeTaskKill(kTargetOpenVpn);
                setCurrentStateAndEmitDisconnected(STATUS_DISCONNECTED);
                return;
            }
            else if (serverReply.contains("Initialization Sequence Completed With Errors", Qt::CaseInsensitive))
            {
                emit error(CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS);
            }
#if defined (Q_OS_MACOS)
            else if (serverReply.contains("Opened") && serverReply.contains("device"))
#elif defined (Q_OS_LINUX)
            else if (serverReply.contains("TUN/TAP device") || serverReply.contains("DCO device"))
#endif
#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
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
        stateVariables_.bSigTermSent = true;
    }
}

void OpenVPNConnection::emitAuthErrorAndSigTerm(boost::system::error_code &write_error)
{
    emit error(CONNECT_ERROR::AUTH_ERROR);
    if (!stateVariables_.bSigTermSent)
    {
        boost::asio::write(*stateVariables_.socket, boost::asio::buffer("signal SIGTERM\n"), boost::asio::transfer_all(), write_error);
        stateVariables_.bSigTermSent = true;
    }
}

void OpenVPNConnection::continueWithUsernameImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "username \"Auth\" \"%s\"\n", sanitizeString(username_).toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message,strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::continueWithPasswordImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "password \"Auth\" \"%s\"\n", sanitizeString(password_).toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::continueWithPrivKeyPasswordImpl()
{
    boost::system::error_code write_error;
    char message[1024];
    snprintf(message, 1024, "password \"Private Key\" \"%s\"\n", sanitizeString(privKeyPassword_).toUtf8().data());
    boost::asio::write(*stateVariables_.socket, boost::asio::buffer(message, strlen(message)), boost::asio::transfer_all(), write_error);

    checkErrorAndContinue(write_error, false);
}

void OpenVPNConnection::setPrivKeyPassword(const QString &password)
{
    privKeyPassword_ = password;
}

// Sanitize a string for use in an OpenVPN management interface command.
// Newlines and carriage returns are stripped (they would break the line-oriented protocol),
// and backslashes, double-quotes, and tabs are escaped per the OpenVPN management-notes.txt
// "Command Parsing" rules.  The result is safe to embed inside a quoted argument.
QString OpenVPNConnection::sanitizeString(const QString &s)
{
    QString result = s;
    result.remove('\r');
    result.remove('\n');
    result.replace("\\", "\\\\");
    result.replace("\"", "\\\"");
    result.replace("\t", "\\t");
    return result;
}

bool OpenVPNConnection::parsePushReply(const QString &reply, AdapterGatewayInfo &outConnectionAdapterInfo, bool &outRedirectDefaultGateway)
{
    int firstQuote = reply.indexOf('\'');
    int lastQuote = reply.lastIndexOf('\'');
    if (firstQuote == -1 || lastQuote == -1)
    {
        return false;
    }

    const QString str2 = reply.mid(firstQuote + 1, lastQuote - firstQuote - 1);
    const QStringList values = str2.split(',');

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
            const QStringList v = it.split(' ');
            if (v.count() != 2)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse route-gateway message";
                return false;
            }
            else
            {
                const QString ipStr = v[1];
                if (!NetworkingValidation::isIp(ipStr))
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse route-gateway message (incorrect IPv4 address)";
                    return false;
                }
                else
                {
                    outConnectionAdapterInfo.addGatewayIp(types::IpAddress(ipStr.toStdString()));
                }
            }
        }
        else if (it.contains("ifconfig", Qt::CaseInsensitive))
        {
            const QStringList v = it.split(' ');
            if (v.count() != 3)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse ifconfig message";
                return false;
            }
            else
            {
                const QString ipStr = v[1];
                if (!NetworkingValidation::isIp(ipStr))
                {
                    qCCritical(LOG_CONNECTION) << "Can't parse ifconfig message (incorrect IPv4 address)";
                    return false;
                }
                else
                {
                    outConnectionAdapterInfo.addAdapterIp(types::IpAddress(ipStr.toStdString()));
                }
            }
        }
        else if (it.contains("dhcp-option", Qt::CaseInsensitive))
        {
            const QStringList v = it.split(' ');
            if (v.count() != 3)
            {
                qCCritical(LOG_CONNECTION) << "Can't parse dhcp-option message";
                return false;
            }
            else
            {
                if (v[1] == QLatin1String("DNS"))
                {
                    const QString ipStr = v[2];
                    if (!NetworkingValidation::isIp(ipStr))
                    {
                        qCCritical(LOG_CONNECTION) << "Can't parse dhcp-option DNS message (incorrect IPv4 address)";
                        return false;
                    }
                    else
                    {
                        outConnectionAdapterInfo.addDnsServer(types::IpAddress(ipStr.toStdString()));
                    }
                }
            }
        }
    }
    return true;
}

bool OpenVPNConnection::parseDeviceOpenedReply(const QString &reply, QString &outDeviceName)
{
    const QStringList v = reply.split(',');
    if (v.count() < 3)
    {
        qCCritical(LOG_CONNECTION) << "Can't parse opened device message";
        return false;
    }
    const QStringList words = v.last().split(' ');
    int deviceIdx = words.indexOf("device");
    if (deviceIdx < 0) {
        deviceIdx = words.indexOf("Device");
    }
    if (deviceIdx < 0 || deviceIdx + 1 >= words.count())
    {
        qCCritical(LOG_CONNECTION) << "Can't find 'device' keyword in opened device message";
        return false;
    }
    outDeviceName = words[deviceIdx + 1];

    WS_ASSERT(outDeviceName.contains("tun"));
    return !outDeviceName.isEmpty();
}

bool OpenVPNConnection::parseConnectedSuccessReply(const QString &reply, QString &outRemoteIp)
{
    const QStringList v = reply.split(',');
    if (v.count() < 5)
    {
        qCCritical(LOG_CONNECTION) << "Can't parse CONNECT SUCCESS message (incorrect number of fields:" << v.count() << ")";
        return false;
    }
    else
    {
        outRemoteIp = v[4];
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
