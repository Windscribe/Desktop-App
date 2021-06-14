#include "helper_linux.h"

Helper_linux::Helper_linux(QObject *parent) : IHelper(parent)
{
}

Helper_linux::~Helper_linux()
{
}

void Helper_linux::startInstallHelper()
{
}


QString Helper_linux::executeRootCommand(const QString &commandLine)
{
    return "";
}

bool Helper_linux::executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName)
{
    Q_UNUSED(commandLine);
    Q_UNUSED(outCmdId);
    Q_UNUSED(eventName);
    return false;
}

bool Helper_linux::executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId)
{
    return false;
}

bool Helper_linux::executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId)
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

bool Helper_linux::executeTaskKill(const QString &executableName)
{
    return false;
}

bool Helper_linux::executeResetTap(const QString &tapName)
{
    Q_UNUSED(tapName);
    return false;
}

QString Helper_linux::executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber)
{
    Q_UNUSED(interfaceType);
    Q_UNUSED(interfaceName);
    Q_UNUSED(metricNumber);
    return "";
}

QString Helper_linux::executeWmicEnable(const QString &adapterName)
{
    Q_UNUSED(adapterName);
    return "";
}

QString Helper_linux::executeWmicGetConfigManagerErrorCode(const QString &adapterName)
{
    Q_UNUSED(adapterName);
    return "";
}

bool Helper_linux::executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid, unsigned long &outCmdId, const QString &eventName)
{
    Q_UNUSED(cmd);
    Q_UNUSED(configPath);
    Q_UNUSED(publicGuid);
    Q_UNUSED(privateGuid);
    Q_UNUSED(outCmdId);
    Q_UNUSED(eventName);
    return false;
}

bool Helper_linux::executeChangeMtu(const QString & /*adapter*/, int /*mtu*/)
{
    // nothing to do on linux
    return false;
}

bool Helper_linux::clearDnsOnTap()
{
    // nothing to do on linux
    return false;
}

QString Helper_linux::enableBFE()
{
    // nothing to do on linux
    return "";
}

QString Helper_linux::resetAndStartRAS()
{
    // nothing to do on linux
    return "";
}

void Helper_linux::setIPv6EnabledInFirewall(bool /*b*/)
{
    // nothing to do on linux
}

void Helper_linux::setIPv6EnabledInOS(bool /*b*/)
{
    // nothing to do on linux
    Q_ASSERT(false);
}

bool Helper_linux::IPv6StateInOS()
{
    //Q_ASSERT(false); // TODO: re-enable and ensure not called
    return false;
}

bool Helper_linux::isHelperConnected()
{
    return true;
}

bool Helper_linux::isFailedConnectToHelper()
{
    return false;
}

void Helper_linux::setNeedFinish()
{

}

QString Helper_linux::getHelperVersion()
{
    return "";
}

void Helper_linux::enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress)
{
}

void Helper_linux::enableFirewallOnBoot(bool bEnable)
{
}

bool Helper_linux::removeWindscribeUrlsFromHosts()
{
    return true;
}

bool Helper_linux::addHosts(const QString &hosts)
{
    Q_UNUSED(hosts);
    return false;
}

bool Helper_linux::removeHosts()
{
    return false;
}

bool Helper_linux::reinstallHelper()
{
    return false;
}

bool Helper_linux::enableWanIkev2()
{
    //nothing todo for linux
    return false;
}

void Helper_linux::closeAllTcpConnections(bool /*isKeepLocalSockets*/)
{
    //nothing todo for linux
}

QStringList Helper_linux::getProcessesList()
{
    //nothing todo for linux
    return QStringList();
}

bool Helper_linux::whitelistPorts(const QString & /*ports*/)
{
    //nothing todo for linux
    return true;
}

bool Helper_linux::deleteWhitelistPorts()
{
    //nothing todo for linux
    return true;
}

void Helper_linux::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
}

void Helper_linux::clearUnblockingCmd(unsigned long cmdId)
{
    Q_UNUSED(cmdId);
}

void Helper_linux::suspendUnblockingCmd(unsigned long cmdId)
{
    // On Linux, this is the same as clearing a cmd.
    clearUnblockingCmd(cmdId);
}

void Helper_linux::enableDnsLeaksProtection()
{
    // nothing todo for Linux
}

void Helper_linux::disableDnsLeaksProtection()
{
    // nothing todo for Linux
}

bool Helper_linux::reinstallWanIkev2()
{
    // nothing todo for Linux
    return false;
}

QStringList Helper_linux::getActiveNetworkInterfaces_mac()
{
    return QStringList();
}

bool Helper_linux::setKeychainUsernamePassword(const QString &username, const QString &password)
{
    return false;
}

bool Helper_linux::setMacAddressRegistryValueSz(QString /*subkeyInterfaceName*/, QString /*value*/)
{
    // nothing for Linux
    return false;
}

bool Helper_linux::removeMacAddressRegistryProperty(QString /*subkeyInterfaceName*/)
{
    // nothing for Linux
    return false;
}

bool Helper_linux::resetNetworkAdapter(QString /*subkeyInterfaceName*/, bool /*bringAdapterBackUp*/)
{
    // nothing for Linux
    return false;
}

bool Helper_linux::addIKEv2DefaultRoute()
{
    // nothing for Linux
    return false;
}

bool Helper_linux::removeWindscribeNetworkProfiles()
{
    // nothing for Linux
    return false;
}

void Helper_linux::setIKEv2IPSecParameters()
{
    // nothing for Linux
}

bool Helper_linux::setSplitTunnelingSettings(bool isActive, bool isExclude,
                                           bool /*isKeepLocalSockets*/, const QStringList &files,
                                           const QStringList &ips, const QStringList &hosts)
{
    return false;
}

void Helper_linux::sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const ProtocolType &protocol)
{
    Q_UNUSED(isCloseTcpSocket);
    Q_UNUSED(isKeepLocalSocket);
}

bool Helper_linux::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    Q_UNUSED(ifIndex)
    Q_UNUSED(overrideDnsIpAddress)

    return false;
}

bool Helper_linux::setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &entry)
{
    return false;
}


bool Helper_linux::setKextPath(const QString &kextPath)
{
    return false;
}

bool Helper_linux::startWireGuard(const QString &exeName, const QString &deviceName)
{
    return false;
}

bool Helper_linux::stopWireGuard()
{
    return true;
}

bool Helper_linux::configureWireGuard(const WireGuardConfig &config)
{
    return false;
}

bool Helper_linux::getWireGuardStatus(WireGuardStatus *status)
{
    return true;
}

void Helper_linux::setDefaultWireGuardDeviceName(const QString &deviceName)
{
}

void Helper_linux::run()
{
}
