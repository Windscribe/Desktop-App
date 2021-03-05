#include "helper_mac.h"
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
#include "../mac/helper/src/ipc/helper_commands_serialize.h"
#include "engine/types/wireguardconfig.h"
#include "engine/types/wireguardtypes.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/types/protocoltype.h"

#define SOCK_PATH "/var/run/windscribe_helper_socket2"

using namespace boost::asio;

Helper_mac *g_this_ = NULL;

Helper_mac::Helper_mac(QObject *parent) : IHelper(parent), bIPV6State_(true), cmdId_(0), lastOpenVPNCmdId_(0), bFailedConnectToHelper_(false), ep_(SOCK_PATH), bHelperConnectedEmitted_(false),
    bHelperConnected_(false), bNeedFinish_(false)
{
    Q_ASSERT(g_this_ == NULL);
    g_this_ = this;
}

Helper_mac::~Helper_mac()
{
    io_service_.stop();
    setNeedFinish();
    wait();
    g_this_ = NULL;
}

void Helper_mac::startInstallHelper()
{
    if (InstallHelper_mac::installHelper())
    {
        start(QThread::LowPriority);
    }
    else
    {
        bFailedConnectToHelper_ = true;
    }
}


QString Helper_mac::executeRootCommand(const QString &commandLine)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
    {
        return "";
    }

    bool bExecuted;
    QString log;
    int ret = executeRootCommandImpl(commandLine, &bExecuted, log);
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
            if (isHelperConnected())
            {
                ret = executeRootCommandImpl(commandLine, &bExecuted, log);
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

bool Helper_mac::executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName)
{
    Q_UNUSED(commandLine);
    Q_UNUSED(outCmdId);
    Q_UNUSED(eventName);
    return false;
}

bool Helper_mac::executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
    {
        return false;
    }

    // get path to openvpn util
    QString strOpenVpnPath = "/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();

    QString helpersPath = QCoreApplication::applicationDirPath() + "/../Helpers";
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
        else
        {
            outCmdId = answerCmd.cmdId;
            return true;
        }
    }
}

bool Helper_mac::executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId)
{
    Q_UNUSED(configPath);
    Q_UNUSED(portNumber);
    Q_UNUSED(httpProxy);
    Q_UNUSED(httpPort);
    Q_UNUSED(socksProxy);
    Q_UNUSED(socksPort);
    Q_UNUSED(outCmdId);
    return false;
}

bool Helper_mac::executeTaskKill(const QString &executableName)
{
    QString killCmd = "pkill " + executableName;
    executeRootCommand(killCmd);
    return true;
}

bool Helper_mac::executeResetTap(const QString &tapName)
{
    Q_UNUSED(tapName);
    return false;
}

QString Helper_mac::executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber)
{
    Q_UNUSED(interfaceType);
    Q_UNUSED(interfaceName);
    Q_UNUSED(metricNumber);
    return "";
}

QString Helper_mac::executeWmicEnable(const QString &adapterName)
{
    Q_UNUSED(adapterName);
    return "";
}

QString Helper_mac::executeWmicGetConfigManagerErrorCode(const QString &adapterName)
{
    Q_UNUSED(adapterName);
    return "";
}

bool Helper_mac::executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid, unsigned long &outCmdId, const QString &eventName)
{
    Q_UNUSED(cmd);
    Q_UNUSED(configPath);
    Q_UNUSED(publicGuid);
    Q_UNUSED(privateGuid);
    Q_UNUSED(outCmdId);
    Q_UNUSED(eventName);
    return false;
}

bool Helper_mac::executeChangeMtu(const QString & /*adapter*/, int /*mtu*/)
{
    // nothing to do on mac
    return false;
}

bool Helper_mac::clearDnsOnTap()
{
    // nothing to do on mac
    return false;
}

QString Helper_mac::enableBFE()
{
    // nothing to do on mac
    return "";
}

QString Helper_mac::resetAndStartRAS()
{
    // nothing to do on mac
    return "";
}

void Helper_mac::setIPv6EnabledInFirewall(bool /*b*/)
{
    // nothing to do on mac
}

void Helper_mac::setIPv6EnabledInOS(bool /*b*/)
{
    // nothing to do on mac
    Q_ASSERT(false);
}

bool Helper_mac::IPv6StateInOS()
{
    //Q_ASSERT(false); // TODO: re-enable and ensure not called
    return false;
}

bool Helper_mac::isHelperConnected()
{
    QMutexLocker locker(&mutexHelperConnected_);
    return bHelperConnected_;
}

QString Helper_mac::getHelperVersion()
{
    return "";
}

void Helper_mac::enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress)
{
    qCDebug(LOG_BASIC) << "Enable MAC spoofing on boot, bEnable =" << bEnable;
    QString strTempFilePath = QString::fromLocal8Bit(getenv("TMPDIR")) + "windscribetemp.plist";
    QString filePath = "/Library/LaunchDaemons/com.aaa.windscribe.macspoofing_on.plist";

    QString macSpoofingScript = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/windscribe_macspoof.sh";

    if (bEnable)
    {
        // bash script
        {
            QString exePath = QCoreApplication::applicationFilePath();
            QFile file(macSpoofingScript);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                file.resize(0);
                QTextStream in(&file);
                in << "#!/bin/bash\n";
                in << "FILE=\"" << exePath << "\"\n";
                in << "if [ ! -f \"$FILE\" ]\n";
                in << "then\n";
                in << "echo \"File $FILE does not exists\"\n";
                in << "launchctl stop com.aaa.windscribe.macspoofing_on\n";
                in << "launchctl unload " << filePath << "\n";
                in << "launchctl remove com.aaa.windscribe.macspoofing_on\n";
                in << "srm \"$0\"\n";
                //in << "rm " << filePath << "\n";
                in << "else\n";
                in << "echo \"File $FILE exists\"\n";
                in << "ipconfig waitall\n";
                in << "sleep 3\n";
                in << "/sbin/ifconfig " << interfaceName << " ether " << macAddress << "\n";
                in << "fi\n";
                file.close();

                // set executable flag
                executeRootCommand("chmod +x \"" + macSpoofingScript + "\"");
            }
        }

        // create plist
        QFile file(strTempFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.resize(0);
            QTextStream in(&file);
            in << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            in << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            in << "<plist version=\"1.0\">\n";
            in << "<dict>\n";
            in << "<key>Label</key>\n";
            in << "<string>com.aaa.windscribe.macspoofing_on</string>\n";

            in << "<key>ProgramArguments</key>\n";
            in << "<array>\n";
            in << "<string>/bin/bash</string>\n";
            in << "<string>" << macSpoofingScript << "</string>\n";
            in << "</array>\n";

            in << "<key>StandardErrorPath</key>\n";
            in << "<string>/var/log/windscribe_macspoofing.log</string>\n";
            in << "<key>StandardOutPath</key>\n";
            in << "<string>/var/log/windscribe_macspoofing.log</string>\n";

            in << "<key>RunAtLoad</key>\n";
            in << "<true/>\n";
            in << "</dict>\n";
            in << "</plist>\n";

            file.close();

            executeRootCommand("cp " + strTempFilePath + " " + filePath);
            executeRootCommand("launchctl load -w " + filePath);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Can't create plist file for startup firewall: " << filePath;
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "launchctl unload " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(macSpoofingScript);
        executeRootCommand("rm \"" + macSpoofingScript + "\"");
    }
}

void Helper_mac::enableFirewallOnBoot(bool bEnable)
{
    qCDebug(LOG_BASIC) << "Enable firewall on boot, bEnable =" << bEnable;
    QString strTempFilePath = QString::fromLocal8Bit(getenv("TMPDIR")) + "windscribetemp.plist";
    QString filePath = "/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist";

    QString pfConfFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString pfBashScriptFile = pfConfFilePath + "/windscribe_pf.sh";
    pfConfFilePath = pfConfFilePath + "/pf.conf";

    if (bEnable)
    {
        //create bash script
        {
            QString exePath = QCoreApplication::applicationFilePath();
            QFile file(pfBashScriptFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                file.resize(0);
                QTextStream in(&file);
                in << "#!/bin/bash\n";
                in << "FILE=\"" << exePath << "\"\n";
                in << "if [ ! -f \"$FILE\" ]\n";
                in << "then\n";
                in << "echo \"File $FILE does not exists\"\n";
                in << "launchctl stop com.aaa.windscribe.firewall_on\n";
                in << "launchctl unload " << filePath << "\n";
                in << "launchctl remove com.aaa.windscribe.firewall_on\n";
                in << "srm \"$0\"\n";
                //in << "rm " << filePath << "\n";
                in << "else\n";
                in << "echo \"File $FILE exists\"\n";
                in << "ipconfig waitall\n";
                in << "/sbin/pfctl -e -f \"" << pfConfFilePath << "\"\n";
                in << "fi\n";
                file.close();

                // set executable flag
                executeRootCommand("chmod +x \"" + pfBashScriptFile + "\"");
            }
        }

        // create plist
        QFile file(strTempFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.resize(0);
            QTextStream in(&file);
            in << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            in << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            in << "<plist version=\"1.0\">\n";
            in << "<dict>\n";
            in << "<key>Label</key>\n";
            in << "<string>com.aaa.windscribe.firewall_on</string>\n";

            in << "<key>ProgramArguments</key>\n";
            in << "<array>\n";
            in << "<string>/bin/bash</string>\n";
            in << "<string>" << pfBashScriptFile << "</string>\n";
            in << "</array>\n";

            in << "<key>StandardErrorPath</key>\n";
            in << "<string>/var/log/windscribe_pf.log</string>\n";
            in << "<key>StandardOutPath</key>\n";
            in << "<string>/var/log/windscribe_pf.log</string>\n";

            in << "<key>RunAtLoad</key>\n";
            in << "<true/>\n";
            in << "</dict>\n";
            in << "</plist>\n";

            file.close();

            executeRootCommand("cp " + strTempFilePath + " " + filePath);
            executeRootCommand("launchctl load -w " + filePath);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Can't create plist file for startup firewall: " << filePath;
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "launchctl unload " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + Utils::cleanSensitiveInfo(filePath);
        executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(pfBashScriptFile);
        executeRootCommand("rm \"" + pfBashScriptFile + "\"");
    }
}

bool Helper_mac::removeWindscribeUrlsFromHosts()
{
    executeRootCommand(TempScripts_mac::instance().removeHostsScriptPath() + " -removewindscribehosts");
    return true;
}

bool Helper_mac::addHosts(const QString &hosts)
{
    Q_UNUSED(hosts);
    return false;
}

bool Helper_mac::removeHosts()
{
    return false;
}

bool Helper_mac::reinstallHelper()
{
    QString strUninstallUtilPath = QCoreApplication::applicationDirPath() + "/../Resources/uninstallHelper.sh";
    InstallHelper_mac::runScriptWithAdminRights(strUninstallUtilPath);
    return true;
}

bool Helper_mac::enableWanIkev2()
{
    //nothing todo for Mac
    return false;
}

void Helper_mac::closeAllTcpConnections(bool /*isKeepLocalSockets*/)
{
    //nothing todo for Mac
}

QStringList Helper_mac::getProcessesList()
{
    //nothing todo for Mac
    return QStringList();
}

bool Helper_mac::whitelistPorts(const QString & /*ports*/)
{
    //nothing todo for Mac
    return true;
}

bool Helper_mac::deleteWhitelistPorts()
{
    //nothing todo for Mac
    return true;
}

void Helper_mac::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    QMutexLocker locker(&mutex_);

    outFinished = false;
    if (!isHelperConnected())
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

void Helper_mac::clearUnblockingCmd(unsigned long cmdId)
{
    Q_UNUSED(cmdId);

    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
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

void Helper_mac::suspendUnblockingCmd(unsigned long cmdId)
{
    // On Mac, this is the same as clearing a cmd.
    clearUnblockingCmd(cmdId);
}

void Helper_mac::enableDnsLeaksProtection()
{
    // nothing todo for Mac
}

void Helper_mac::disableDnsLeaksProtection()
{
    // nothing todo for Mac
}

bool Helper_mac::reinstallWanIkev2()
{
    // nothing todo for Mac
    return false;
}

QStringList Helper_mac::getActiveNetworkInterfaces_mac()
{
    const QString answer = executeRootCommand("ifconfig -a");
    const QStringList lines = answer.split("\n");
    QStringList res;
    for (const QString s : lines)
    {
        if (s.startsWith("en", Qt::CaseInsensitive))
        {
            int ind = s.indexOf(':');
            if (ind != -1)
            {
                res << s.left(ind);
            }
        }
    }

    return res;
}

bool Helper_mac::setKeychainUsernamePassword(const QString &username, const QString &password)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
    {
        return false;
    }

    bool bExecuted;
    int ret = setKeychainUsernamePasswordImpl(username, password, &bExecuted);
    if (ret == RET_SUCCESS)
    {
        return bExecuted;
    }
    else
    {
        qCDebug(LOG_BASIC) << "App disconnected from helper, try reconnect";
        doDisconnectAndReconnect();

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        while (elapsedTimer.elapsed() < MAX_WAIT_HELPER && !isNeedFinish())
        {
            if (isHelperConnected())
            {
                ret = setKeychainUsernamePasswordImpl(username, password, &bExecuted);
                if (ret == RET_SUCCESS)
                {
                    return bExecuted;
                }
            }
            msleep(1);
        }
    }

    qCDebug(LOG_BASIC) << "setKeychainUsernamePassword() failed";
    return false;
}

bool Helper_mac::setMacAddressRegistryValueSz(QString /*subkeyInterfaceName*/, QString /*value*/)
{
    // nothing for mac
    return false;
}

bool Helper_mac::removeMacAddressRegistryProperty(QString /*subkeyInterfaceName*/)
{
    // nothing for mac
    return false;
}

bool Helper_mac::resetNetworkAdapter(QString /*subkeyInterfaceName*/, bool /*bringAdapterBackUp*/)
{
    // nothing for mac
    return false;
}

bool Helper_mac::addIKEv2DefaultRoute()
{
    // nothing for mac
    return false;
}

bool Helper_mac::removeWindscribeNetworkProfiles()
{
    // nothing for mac
    return false;
}

void Helper_mac::setIKEv2IPSecParameters()
{
    // nothing for mac
}

bool Helper_mac::setSplitTunnelingSettings(bool isActive, bool isExclude,
                                           bool /*isKeepLocalSockets*/, const QStringList &files,
                                           const QStringList &ips, const QStringList &hosts)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
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

void Helper_mac::sendConnectStatus(bool isConnected, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const ProtocolType &protocol)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
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

bool Helper_mac::setKextPath(const QString &kextPath)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
    {
        return false;
    }

    CMD_SET_KEXT_PATH cmd;
    cmd.kextPath = kextPath.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_SET_KEXT_PATH, stream.str()))
    {
        return false;
    }
    else
    {
        CMD_ANSWER answerCmd;
        if (!readAnswer(answerCmd))
        {
            return false;
        }
    }
    return true;
}

bool Helper_mac::startWireGuard(const QString &exeName, const QString &deviceName)
{
    // Make sure the device socket is closed.
    auto result = executeRootCommand("rm -f /var/run/wireguard/" + deviceName + ".sock");
    if (!result.isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Helper_mac: sock rm failed:" << result;
        return false;
    }

    QMutexLocker locker(&mutex_);
    wireGuardExeName_ = exeName;
    wireGuardDeviceName_.clear();

    if (!isHelperConnected())
        return false;

    CMD_START_WIREGUARD cmd;
    cmd.exePath = (QCoreApplication::applicationDirPath() + "/../Helpers/" + exeName).toStdString();
    cmd.deviceName = deviceName.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_START_WIREGUARD, stream.str())) {
        doDisconnectAndReconnect();
        return false;
    }
    CMD_ANSWER answerCmd;
    if (!readAnswer(answerCmd)) {
        doDisconnectAndReconnect();
        return false;
    }

    if (answerCmd.executed)
        wireGuardDeviceName_ = deviceName;

    return answerCmd.executed != 0;
}

bool Helper_mac::stopWireGuard()
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

    if (isHelperConnected()) {
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

bool Helper_mac::configureWireGuard(const WireGuardConfig &config)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
        return false;

    CMD_CONFIGURE_WIREGUARD cmd;
    cmd.clientPrivateKey =
        QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().data();
    cmd.clientIpAddress = config.clientIpAddress().toLatin1().data();
    cmd.clientDnsAddressList = config.clientDnsAddress().toLatin1().data();
    cmd.clientDnsScriptName = TempScripts_mac::instance().dnsScriptPath().toLatin1().data();
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

bool Helper_mac::getWireGuardStatus(WireGuardStatus *status)
{
    QMutexLocker locker(&mutex_);

    if (status) {
        status->state = WireGuardState::NONE;
        status->errorCode = 0;
        status->bytesReceived = status->bytesTransmitted = 0;
    }
    if (!isHelperConnected())
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

void Helper_mac::setDefaultWireGuardDeviceName(const QString &deviceName)
{
    // If we don't have an active WireGuard device, assign the default device name. It is important
    // for a subsequent call to stopWireGuard(), to stop the device created during the last session.
    if (wireGuardDeviceName_.isEmpty())
        wireGuardDeviceName_ = deviceName;
}

int Helper_mac::executeRootCommandImpl(const QString &commandLine, bool *bExecuted, QString &answer)
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
            return RET_SUCCESS;
        }
    }
}

bool Helper_mac::setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted)
{
    CMD_SET_KEYCHAIN_ITEM cmd;
    cmd.username = username.toStdString();
    cmd.password = password.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!sendCmdToHelper(HELPER_CMD_SET_KEYCHAIN_ITEM, stream.str()))
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
            *bExecuted = (answerCmd.executed == 1);
            return RET_SUCCESS;
        }
    }
}

void Helper_mac::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    io_service_.reset();
    reconnectElapsedTimer_.start();
    g_this_->socket_.reset(new boost::asio::local::stream_protocol::socket(io_service_));
    socket_->async_connect(ep_, connectHandler);
    io_service_.run();
}

void Helper_mac::connectHandler(const boost::system::error_code &ec)
{
    if (!ec)
    {
        // we connected
        g_this_->setHelperConnected(true);
        //emit signal only once on first run
        if (!g_this_->bHelperConnectedEmitted_)
        {
            g_this_->bHelperConnectedEmitted_ = true;
        }
        qCDebug(LOG_BASIC) << "connected to helper socket";
    }
    else
    {
        if (g_this_->reconnectElapsedTimer_.elapsed() > MAX_WAIT_HELPER)
        {
            if (!g_this_->bHelperConnectedEmitted_)
            {
                g_this_->bFailedConnectToHelper_ = true;
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

void Helper_mac::doDisconnectAndReconnect()
{
    if (!isRunning())
    {
        qCDebug(LOG_BASIC) << "Disconnected from helper socket, try reconnect";
        g_this_->setHelperConnected(false);
        start(QThread::LowPriority);
    }
}

bool Helper_mac::readAnswer(CMD_ANSWER &outAnswer)
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

bool Helper_mac::sendCmdToHelper(int cmdId, const std::string &data)
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
