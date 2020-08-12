#include "helper_mac.h"
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
#include "engine/splittunnelingnetworkinfo/splittunnelingnetworkinfo.h"


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

    qDebug() << cmdOpenVPN;

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
    Q_UNUSED(executableName);
    return false;
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

bool Helper_mac::executeChangeMtu(const QString &adapter, int mtu)
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

void Helper_mac::setIPv6EnabledInFirewall(bool b)
{
    Q_UNUSED(b);
}

void Helper_mac::setIPv6EnabledInOS(bool b)
{
    Q_UNUSED(b);
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
        qCDebug(LOG_BASIC) << "Execute command: " << "launchctl unload " + filePath;
        executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + filePath;
        executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + macSpoofingScript;
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
        qCDebug(LOG_BASIC) << "Execute command: " << "launchctl unload " + filePath;
        executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + filePath;
        executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + pfBashScriptFile;
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

bool Helper_mac::whitelistPorts(const QString &ports)
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
    QString answer = executeRootCommand("ifconfig -a");
    QStringList lines = answer.split("\n");
    QStringList res;
    Q_FOREACH(const QString s, lines)
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

bool Helper_mac::setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value)
{
    // nothing for mac
    return false;
}

bool Helper_mac::removeMacAddressRegistryProperty(QString subkeyInterfaceName)
{
    // nothing for mac
    return false;
}

bool Helper_mac::resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp)
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

void Helper_mac::sendConnectStatus(bool isConnected, const SplitTunnelingNetworkInfo &stni)
{
    QMutexLocker locker(&mutex_);

    if (!isHelperConnected())
    {
        return;
    }

    CMD_SEND_CONNECT_STATUS cmd;

    if (isConnected)
    {
        if (stni.protocol().isStunnelOrWStunnelProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL;
        }
        else if (stni.protocol().isIkev2Protocol())
        {
            cmd.protocol = CMD_PROTOCOL_IKEV2;
        }
        else if (stni.protocol().isOpenVpnProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_OPENVPN;
        }
        else
        {
            Q_ASSERT(false);
        }
    }

    cmd.isConnected = isConnected;
    cmd.connectedIp = stni.connectedIp().toStdString();
    cmd.gatewayIp = stni.gatewayIp().toStdString();
    cmd.interfaceName = stni.interfaceName().toStdString();
    cmd.interfaceIp = stni.interfaceIp().toStdString();
    cmd.routeVpnGateway = stni.routeVpnGateway().toStdString();
    cmd.routeNetGateway = stni.routeNetGateway().toStdString();
    cmd.remote_1 = stni.remote1().toStdString();
    cmd.ifconfigTunIp = stni.ifconfigTunIp().toStdString();

    cmd.ikev2AdapterAddress = stni.ikev2AdapterAddress().toStdString();

    cmd.vpnAdapterName = stni.vpnAdapterName().toStdString();

    Q_FOREACH(const QString &dns, stni.dnsServers())
    {
        cmd.dnsServers.push_back(dns.toStdString());
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

    boost::asio::write(*socket_, boost::asio::buffer(&cmdId, sizeof(cmdId)), boost::asio::transfer_exactly(sizeof(cmdId)), ec);
    if (ec)
    {
        return false;
    }
    boost::asio::write(*socket_, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec)
    {
        return false;
    }

    boost::asio::write(*socket_, boost::asio::buffer(data.data(), length), boost::asio::transfer_exactly(length), ec);
    if (ec)
    {
        doDisconnectAndReconnect();
        return false;
    }

    return true;
}

