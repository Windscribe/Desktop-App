#pragma once

#include "helper_posix.h"

// Linux commands
class Helper_linux : public Helper_posix
{
public:
    // Take ownership of the backend
    explicit Helper_linux(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger);

    void setDnsLeakProtectEnabled(bool bEnabled);
    void setGaiIpv4PriorityEnabled(bool bEnabled);
    void resetMacAddresses(const QString &ignoreNetwork = "");
    bool setMacAddress(const QString &interface, const QString &macAddress, const QString &network = "", bool isWifi = false);
    void setOpenVpnDcoMode(bool useDco);

    QString installerStageAndVerify(const QString &srcPath);
};
