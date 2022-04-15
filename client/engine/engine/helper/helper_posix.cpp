#include "helper_posix.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include "utils/utils.h"
#include "engine/tempscripts_mac.h"
#include "../openvpnversioncontroller.h"
#include "installhelper_mac.h"
#include "../../../../backend/posix_common/helper_commands_serialize.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "engine/types/wireguardtypes.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/types/protocoltype.h"
#include "utils/macutils.h"
#include "utils/executable_signature/executable_signature.h"

#ifdef Q_OS_LINUX
    #include "utils/dnsscripts_linux.h"
#endif

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

using namespace boost::asio;

Helper_posix *g_this_ = NULL;

Helper_posix::Helper_posix(QObject *parent) : IHelper(parent), bIPV6State_(true), cmdId_(0), lastOpenVPNCmdId_(0)
  , ep_(SOCK_PATH), bHelperConnectedEmitted_(false)
  , curState_(STATE_INIT), bNeedFinish_(false), firstConnectToHelperErrorReported_(false)
{
    Q_ASSERT(g_this_ == NULL);
    g_this_ = this;
}

Helper_posix::~Helper_posix()
{
    io_service_.stop();
    setNeedFinish();
    wait();
    g_this_ = NULL;
}

Helper_posix::STATE Helper_posix::currentState() const
{
    return curState_;
}

void Helper_posix::setNeedFinish()
{
    bNeedFinish_ = true;
}

void Helper_posix::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    QMutexLocker locker(&mutex_);

    outFinished = false;
    if (curState_ != STATE_CONNECTED)
    {
        return;
    }

    CMD_GET_CMD_STATUS cmd;
    cmd.cmdId = cmdId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_GET_CMD_STATUS, stream.str()))
    {
        doDisconnectAndReconnect();
        return;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            doDisconnectAndReconnect();
            return;
        }
        else
        {
            outFinished = (answerCmd.executed == 1);
            outLog = QString::fromStdString(answerCmd.body);
            return;
        }
    }
}

void Helper_posix::clearUnblockingCmd(unsigned long cmdId)
{
    Q_UNUSED(cmdId);

    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return;
    }

    CMD_CLEAR_CMDS cmd;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_CLEAR_CMDS, stream.str()))
    {
        doDisconnectAndReconnect();
        return;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            doDisconnectAndReconnect();
            return;
        }
    }
}

void Helper_posix::suspendUnblockingCmd(unsigned long cmdId)
{
    // On Mac, this is the same as clearing a cmd.
    clearUnblockingCmd(cmdId);
}

bool Helper_posix::setSplitTunnelingSettings(bool isActive, bool isExclude,
                                           bool /*isKeepLocalSockets*/, const QStringList &files,
                                           const QStringList &ips, const QStringList &hosts)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return false;
    }

    CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
    cmdSplitTunnelingSettings.isActive = isActive;
    cmdSplitTunnelingSettings.isExclude = isExclude;

    for (int i = 0; i < files.count(); ++i)
    {
        cmdSplitTunnelingSettings.files.push_back(files[i].toStdString());
    }

    for (int i = 0; i < ips.count(); ++i)
    {
        cmdSplitTunnelingSettings.ips.push_back(ips[i].toStdString());
    }

    for (int i = 0; i < hosts.count(); ++i)
    {
        cmdSplitTunnelingSettings.hosts.push_back(hosts[i].toStdString());
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSplitTunnelingSettings;

    if (!sendCmdToHelper(HELPER_CMD_SPLIT_TUNNELING_SETTINGS, stream.str()))
    {
        doDisconnectAndReconnect();
        return false;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            doDisconnectAndReconnect();
            return false;
        }
    }

    return true;
}

void Helper_posix::sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const ProtocolType &protocol)
{
    Q_UNUSED(isCloseTcpSocket);
    Q_UNUSED(isKeepLocalSocket);
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return;
    }

    CMD_SEND_CONNECT_STATUS cmd;
    cmd.isConnected = isConnected;

    if (isConnected)
    {
        if (protocol.isStunnelOrWStunnelProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL;
        }
        else if (protocol.isIkev2Protocol())
        {
            cmd.protocol = CMD_PROTOCOL_IKEV2;
        }
        else if (protocol.isWireGuardProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_WIREGUARD;
        }
        else if (protocol.isOpenVpnProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_OPENVPN;
        }
        else
        {
            Q_ASSERT(false);
        }

        auto fillAdapterInfo = [](const AdapterGatewayInfo &a, ADAPTER_GATEWAY_INFO &out)
        {
            out.adapterName = a.adapterName().toStdString();
            out.adapterIp = a.adapterIp().toStdString();
            out.gatewayIp = a.gateway().toStdString();
            const QStringList dns = a.dnsServers();
            for(auto ip : dns)
            {
                out.dnsServers.push_back(ip.toStdString());
            }
        };

        fillAdapterInfo(defaultAdapter, cmd.defaultAdapter);
        fillAdapterInfo(vpnAdapter, cmd.vpnAdapter);

        cmd.connectedIp = connectedIp.toStdString();
        cmd.remoteIp = vpnAdapter.remoteIp().toStdString();
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_SEND_CONNECT_STATUS, stream.str()))
    {
        doDisconnectAndReconnect();
        return;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            doDisconnectAndReconnect();
            return;
        }
    }
}

/*bool Helper_posix::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    Q_UNUSED(ifIndex)
    Q_UNUSED(overrideDnsIpAddress)

    // get list of entries of interest
    QStringList networkServices = MacUtils::getListOfDnsNetworkServiceEntries();

    // filter list to only SetByWindscribe entries
    QStringList dnsNetworkServices;

    if (isIkev2)
    {
        // IKEv2 is slightly different -- look for "ConfirmedServiceID" key in each DNS dictionary
        for (QString service : networkServices)
        {
            if (MacUtils::dynamicStoreEntryHasKey(service, "ConfirmedServiceID"))
            {
                dnsNetworkServices.append(service);
            }
        }
    }
    else
    {
        // WG and openVPN: just look for 'SetByWindscribe' key in each DNS dictionary
        for (QString service : networkServices)
        {
            if (MacUtils::dynamicStoreEntryHasKey(service, "SetByWindscribe"))
            {
                dnsNetworkServices.append(service);
            }
        }
    }
    qCDebug(LOG_CONNECTED_DNS) << "Applying custom 'while connected' DNS change to network services: " << dnsNetworkServices;

    if (dnsNetworkServices.isEmpty())
    {
        qCDebug(LOG_CONNECTED_DNS) << "No network services to confirgure 'while connected' DNS";
        return false;
    }

    // change DNS on each entry
    bool successAll = true;
    for (QString service : dnsNetworkServices)
    {
        if (!Helper_posix::setDnsOfDynamicStoreEntry(overrideDnsIpAddress, service))
        {
            successAll = false;
            qCDebug(LOG_CONNECTED_DNS) << "Failed to set network service DNS: " << service;
            break;
        }
    }

    return successAll;
}
*/

IHelper::ExecuteError Helper_posix::startWireGuard(const QString &exeName, const QString &deviceName)
{
    // Make sure the device socket is closed.
    auto result = executeRootCommand("rm -f /var/run/wireguard/" + deviceName + ".sock");
    if (!result.isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Helper_mac: sock rm failed:" << result;
        return IHelper::EXECUTE_ERROR;
    }

    QMutexLocker locker(&mutex_);

    // check executable signature
    // no need for windows implementation in posix file
#if defined Q_OS_LINUX
    const QString &wireGuardExePath = QCoreApplication::applicationDirPath() + "/" + exeName;
#else
    const QString &wireGuardExePath = QCoreApplication::applicationDirPath() + "/../Helpers/windscribewireguard";
#endif

    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(wireGuardExePath.toStdWString()))
    {
        qCDebug(LOG_CONNECTION) << "WireGuard executable signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        return IHelper::EXECUTE_VERIFY_ERROR;
    }

    wireGuardExeName_ = exeName;
    wireGuardDeviceName_.clear();

    if (curState_ != STATE_CONNECTED)
    {
        return IHelper::EXECUTE_ERROR;
    }

    CMD_START_WIREGUARD cmd;
#ifdef Q_OS_MAC
    cmd.exePath = (QCoreApplication::applicationDirPath() + "/../Helpers/" + exeName).toStdString();
#elif defined Q_OS_LINUX
    cmd.exePath = (QCoreApplication::applicationDirPath() + "/" + exeName).toStdString();
#else
    Q_ASSERT(false);
#endif
    cmd.deviceName = deviceName.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_START_WIREGUARD, stream.str())) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd)) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }

    if (answerCmd.executed)
        wireGuardDeviceName_ = deviceName;

    return answerCmd.executed != 0 ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_posix::stopWireGuard()
{
    if (wireGuardDeviceName_.isEmpty())
        return true;

    auto result = executeRootCommand("rm -f /var/run/wireguard/" + wireGuardDeviceName_ + ".sock");
    if (!result.isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Helper_mac: sock rm failed:" << result;
        return false;
    }
    result = executeRootCommand("pkill -f \"" + wireGuardExeName_ + "\"");
    if (!result.isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Helper_mac: pkill failed:" << result;
        return false;
    }

    if (curState_ == STATE_CONNECTED) {
        QMutexLocker locker(&mutex_);

        if (!sendCmdToHelper(HELPER_CMD_STOP_WIREGUARD, "")) {
            doDisconnectAndReconnect();
            return false;
        }
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd)) {
            doDisconnectAndReconnect();
            return false;
        }
        if (answerCmd.executed == 0)
            return false;
        if (!answerCmd.body.empty()) {
            qCDebugMultiline(LOG_WIREGUARD) << "WireGuard daemon output:"
                                            << QString::fromStdString(answerCmd.body);
            }
    }
    return true;
}

bool Helper_posix::configureWireGuard(const WireGuardConfig &config)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
        return false;

    CMD_CONFIGURE_WIREGUARD cmd;
    cmd.clientPrivateKey =
        QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().data();
    cmd.clientIpAddress = config.clientIpAddress().toLatin1().data();
    cmd.clientDnsAddressList = config.clientDnsAddress().toLatin1().data();
#ifdef Q_OS_MAC
    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
#elif defined(Q_OS_LINUX)
    QString strDnsPath = DnsScripts_linux::instance().scriptPath();
#endif
    if (strDnsPath.isEmpty()) {
        return false;
    }
    cmd.clientDnsScriptName = strDnsPath.toLatin1().data();

    cmd.peerEndpoint = config.peerEndpoint().toLatin1().data();
    cmd.peerPublicKey = QByteArray::fromBase64(config.peerPublicKey().toLatin1()).toHex().data();
    cmd.peerPresharedKey
        = QByteArray::fromBase64(config.peerPresharedKey().toLatin1()).toHex().data();
    cmd.allowedIps = config.peerAllowedIps().toLatin1().data();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_CONFIGURE_WIREGUARD, stream.str())) {
        qCDebug(LOG_WIREGUARD) << "WireGuard configuration failed";
        doDisconnectAndReconnect();
        return false;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd)) {
        doDisconnectAndReconnect();
        return false;
    }
    return answerCmd.executed != 0;
}

bool Helper_posix::getWireGuardStatus(WireGuardStatus *status)
{
    QMutexLocker locker(&mutex_);

    if (status) {
        status->state = WireGuardState::NONE;
        status->errorCode = 0;
        status->bytesReceived = status->bytesTransmitted = 0;
    }
    if (curState_ != STATE_CONNECTED)
        return false;

    if (!sendCmdToHelper(HELPER_CMD_GET_WIREGUARD_STATUS, "")) {
        doDisconnectAndReconnect();
        return false;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd)) {
        doDisconnectAndReconnect();
        return false;
    }

    if (!answerCmd.executed)
        return false;

    switch (answerCmd.cmdId) {
    default:
    case WIREGUARD_STATE_NONE:
        status->state = WireGuardState::NONE;
        break;
    case WIREGUARD_STATE_ERROR:
        status->state = WireGuardState::FAILURE;
        status->errorCode = answerCmd.customInfoValue[0];
        break;
    case WIREGUARD_STATE_STARTING:
        status->state = WireGuardState::STARTING;
        break;
    case WIREGUARD_STATE_LISTENING:
        status->state = WireGuardState::LISTENING;
        break;
    case WIREGUARD_STATE_CONNECTING:
        status->state = WireGuardState::CONNECTING;
        break;
    case WIREGUARD_STATE_ACTIVE:
        status->state = WireGuardState::ACTIVE;
        status->bytesReceived = answerCmd.customInfoValue[0];
        status->bytesTransmitted = answerCmd.customInfoValue[1];
        break;
    }
    if (!answerCmd.body.empty()) {
        qCDebugMultiline(LOG_WIREGUARD) << "WireGuard daemon output:"
                                        << QString::fromStdString(answerCmd.body);
    }
    return true;
}

void Helper_posix::setDefaultWireGuardDeviceName(const QString &deviceName)
{
    // If we don't have an active WireGuard device, assign the default device name. It is important
    // for a subsequent call to stopWireGuard(), to stop the device created during the last session.
    if (wireGuardDeviceName_.isEmpty())
        wireGuardDeviceName_ = deviceName;
}

QString Helper_posix::executeRootCommand(const QString &commandLine, int *exitCode /* = nullptr*/)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
    {
        return "";
    }

    bool bExecuted;
    QString log;
    int ret = executeRootCommandImpl(commandLine, &bExecuted, log, exitCode);
    if (ret == RET_SUCCESS)
    {
        return log;
    }
    else
    {
        qCDebug(LOG_BASIC) << "App disconnected from helper, try reconnect";
        doDisconnectAndReconnect();

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        while (elapsedTimer.elapsed() < MAX_WAIT_HELPER && !isNeedFinish())
        {
            if (curState_ == STATE_CONNECTED)
            {
                ret = executeRootCommandImpl(commandLine, &bExecuted, log, exitCode);
                if (ret == RET_SUCCESS)
                {
                    return log;
                }
            }
            msleep(1);
        }
    }

    qCDebug(LOG_BASIC) << "executeRootCommand() failed";
    return "";
}

IHelper::ExecuteError Helper_posix::executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId)
{
    QMutexLocker locker(&mutex_);

    // check openvpn executable signature
    // no need for windows implementation in posix file
#if defined Q_OS_LINUX
    const QString &openVpnExePath = QCoreApplication::applicationDirPath() + "/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();
#else
    const QString &openVpnExePath = QCoreApplication::applicationDirPath() + "/../Helpers/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();
#endif

    ExecutableSignature sigCheck;
    if (!sigCheck.verifyWithSignCheck(openVpnExePath.toStdWString()))
    {
        qCDebug(LOG_CONNECTION) << "OpenVPN executable signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        return IHelper::EXECUTE_VERIFY_ERROR;
    }

    if (curState_ != STATE_CONNECTED)
    {
        return IHelper::EXECUTE_ERROR;
    }

    // get path to openvpn util
    QString strOpenVpnPath = "/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();

#ifdef Q_OS_MAC
    QString helpersPath = QCoreApplication::applicationDirPath() + "/../Helpers";
#elif defined Q_OS_LINUX
    QString helpersPath = QCoreApplication::applicationDirPath() + "/";
#endif
    QString pathToOpenVPN = helpersPath + strOpenVpnPath;
    QString cmdOpenVPN = QString("cd '%1' && %2 %3").arg(pathToOvpnConfig, pathToOpenVPN, commandLine);

    CMD_EXECUTE_OPENVPN cmd;
    cmd.cmdline = cmdOpenVPN.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    qDebug() << Utils::cleanSensitiveInfo(cmdOpenVPN);

    if (!sendCmdToHelper(HELPER_CMD_EXECUTE_OPENVPN, stream.str()))
    {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            doDisconnectAndReconnect();
            return IHelper::EXECUTE_ERROR;
        }
        else
        {
            outCmdId = answerCmd.cmdId;
            return IHelper::EXECUTE_SUCCESS;
        }
    }
}

bool Helper_posix::executeTaskKill(const QString &executableName)
{
    QString killCmd = "pkill " + executableName;
    executeRootCommand(killCmd);
    return true;
}

int Helper_posix::executeRootCommandImpl(const QString &commandLine, bool *bExecuted, QString &answer, int *exitCode /*= nullptr*/)
{
    CMD_EXECUTE cmd;
    cmd.cmdline = commandLine.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_EXECUTE, stream.str()))
    {
        return RET_DISCONNECTED;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            return RET_DISCONNECTED;
        }
        else
        {
            *bExecuted = true;
            answer = QString::fromStdString(answerCmd.body);
            if (exitCode)
            {
                *exitCode = answerCmd.exitCode;
            }
            return RET_SUCCESS;
        }
    }
}

void Helper_posix::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    firstConnectToHelperErrorReported_ = false;
    io_service_.reset();
    reconnectElapsedTimer_.start();
    g_this_->socket_.reset(new boost::asio::local::stream_protocol::socket(io_service_));
    socket_->async_connect(ep_, connectHandler);
    io_service_.run();
}

void Helper_posix::connectHandler(const boost::system::error_code &ec)
{
    if (!ec)
    {
        // we connected
        g_this_->curState_ = STATE_CONNECTED;
        //emit signal only once on first run
        if (!g_this_->bHelperConnectedEmitted_)
        {
            g_this_->bHelperConnectedEmitted_ = true;
        }
        qCDebug(LOG_BASIC) << "connected to helper socket";
    }
    else
    {
        // only report the first error while connecting to helper in order to prevent log bloat
        // we want to see first error in case it is different than all following errors
        if (!g_this_->firstConnectToHelperErrorReported_)
        {
            qCDebug(LOG_BASIC) << "Error while connecting to helper (first): " << ec.value();
            g_this_->firstConnectToHelperErrorReported_ = true;
        }

        if (g_this_->reconnectElapsedTimer_.elapsed() > MAX_WAIT_HELPER)
        {
            if (!g_this_->bHelperConnectedEmitted_)
            {
                qCDebug(LOG_BASIC) << "Error while connecting to helper: " << ec.value();
                g_this_->curState_ = STATE_FAILED_CONNECT;
            }
            else
            {
                emit g_this_->lostConnectionToHelper();
            }
        }
        else
        {
            // try reconnect
            msleep(10);
            {
                QMutexLocker locker(&g_this_->mutexSocket_);
                g_this_->socket_->async_connect(g_this_->ep_, connectHandler);
            }
        }
    }
}

void Helper_posix::doDisconnectAndReconnect()
{
    if (!isRunning())
    {
        qCDebug(LOG_BASIC) << "Disconnected from helper socket, try reconnect";
        g_this_->curState_ = STATE_INIT;
        start(QThread::LowPriority);
    }
}

bool Helper_posix::readAnswer(CMD_ANSWER &outAnswer)
{
    boost::system::error_code ec;
    int length;
    boost::asio::read(*socket_, boost::asio::buffer(&length, sizeof(length)),
                      boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec)
    {
        return false;
    }
    else
    {
        std::vector<char> buff(length);
        boost::asio::read(*socket_, boost::asio::buffer(&buff[0], length),
                          boost::asio::transfer_exactly(length), ec);
        if (ec)
        {
            return false;
        }

        std::string str(buff.begin(), buff.end());
        std::istringstream stream(str);
        boost::archive::text_iarchive ia(stream, boost::archive::no_header);
        ia >> outAnswer;
    }

    return true;
}

bool Helper_posix::sendCmdToHelper(int cmdId, const std::string &data)
{
    int length = data.size();
    boost::system::error_code ec;

    // first 4 bytes - cmdId
    boost::asio::write(*socket_, boost::asio::buffer(&cmdId, sizeof(cmdId)), boost::asio::transfer_exactly(sizeof(cmdId)), ec);
    if (ec)
    {
        return false;
    }
    // second 4 bytes - pid
    const auto pid = getpid();
    boost::asio::write(*socket_, boost::asio::buffer(&pid, sizeof(pid)), boost::asio::transfer_exactly(sizeof(pid)), ec);
    if (ec)
    {
        return false;
    }
    // third 4 bytes - size of buffer
    boost::asio::write(*socket_, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec)
    {
        return false;
    }
    // body of message
    boost::asio::write(*socket_, boost::asio::buffer(data.data(), length), boost::asio::transfer_exactly(length), ec);
    if (ec)
    {
        doDisconnectAndReconnect();
        return false;
    }

    return true;
}
