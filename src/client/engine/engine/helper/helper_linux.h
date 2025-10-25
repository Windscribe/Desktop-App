#pragma once

#include "helper_posix.h"

// Linux commands
class Helper_linux : public Helper_posix
{
public:
    // Take ownership of the backend
    explicit Helper_linux(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger);

    bool installUpdate(const QString& package) const;
    void setDnsLeakProtectEnabled(bool bEnabled);
    void resetMacAddresses(const QString &ignoreNetwork = "");

    bool startWireGuard();
    bool stopWireGuard();
    bool configureWireGuard(const WireGuardConfig &config);
    bool getWireGuardStatus(types::WireGuardStatus *status);
};
