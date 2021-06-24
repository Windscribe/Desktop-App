#include "helper_linux.h"

Helper_linux::Helper_linux(QObject *parent) : IHelper(parent)
{
}

Helper_linux::~Helper_linux()
{
}

void Helper_linux::startInstallHelper()
{
    start(QThread::LowPriority);
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

bool Helper_linux::reinstallHelper()
{
    return false;
}

void Helper_linux::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{

}

void Helper_linux::clearUnblockingCmd(unsigned long cmdId)
{

}

void Helper_linux::suspendUnblockingCmd(unsigned long cmdId)
{

}

bool Helper_linux::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets, const QStringList &files, const QStringList &ips, const QStringList &hosts)
{
    return false;
}

void Helper_linux::sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter, const QString &connectedIp, const ProtocolType &protocol)
{

}

bool Helper_linux::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    return false;
}

bool Helper_linux::startWireGuard(const QString &exeName, const QString &deviceName)
{
    return false;
}

bool Helper_linux::stopWireGuard()
{
    return false;
}

bool Helper_linux::configureWireGuard(const WireGuardConfig &config)
{
    return false;
}

bool Helper_linux::getWireGuardStatus(WireGuardStatus *status)
{
    return false;
}

void Helper_linux::setDefaultWireGuardDeviceName(const QString &deviceName)
{

}


void Helper_linux::run()
{
}

