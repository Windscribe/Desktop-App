#include "helper_linux.h"

Helper_linux::Helper_linux(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger) : Helper_posix(std::move(backend), logger)
{
}

void Helper_linux::setDnsLeakProtectEnabled(bool bEnabled)
{
    sendCommand(HelperCommand::setDnsLeakProtectEnabled, bEnabled);
}

void Helper_linux::setGaiIpv4PriorityEnabled(bool bEnabled)
{
    sendCommand(HelperCommand::setGaiIpv4PriorityEnabled, bEnabled);
}

void Helper_linux::resetMacAddresses(const QString &ignoreNetwork)
{
    sendCommand(HelperCommand::resetMacAddresses, ignoreNetwork.toStdString());
}

void Helper_linux::setOpenVpnDcoMode(bool useDco)
{
    sendCommand(HelperCommand::setOpenVpnDcoMode, useDco);
}

QString Helper_linux::installerStageAndVerify(const QString &srcPath)
{
    auto result = sendCommand(HelperCommand::installerStageAndVerify, srcPath.toStdString());
    std::string stagedPath;
    bool success = false;
    deserializeAnswer(result, stagedPath, success);
    if (!success) {
        return QString();
    }
    return QString::fromStdString(stagedPath);
}

