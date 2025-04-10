#include "helper_mac.h"

Helper_mac::Helper_mac(std::unique_ptr<IHelperBackend> backend) : Helper_posix(std::move(backend))
{
}

void Helper_mac::setDnsScriptEnabled(bool bEnabled)
{
    sendCommand(HelperCommand::setDnsScriptEnabled, bEnabled);
}

void Helper_mac::enableMacSpoofingOnBoot(bool bEnabled, const QString &interfaceName, const QString &macAddress)
{
    sendCommand(HelperCommand::enableMacSpoofingOnBoot, bEnabled, interfaceName.toStdString(), macAddress.toStdString());
}

bool Helper_mac::setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &entry)
{
    auto result = sendCommand(HelperCommand::setDnsOfDynamicStoreEntry, ipAddress.toStdString(), entry.toStdString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

void Helper_mac::setIpv6Enabled(bool bEnabled)
{
    sendCommand(HelperCommand::setIpv6Enabled, bEnabled);
}

void Helper_mac::deleteRoute(const QString &range, int mask, const QString &gateway)
{
    sendCommand(HelperCommand::deleteRoute, range.toStdString(), mask, gateway.toStdString());
}

