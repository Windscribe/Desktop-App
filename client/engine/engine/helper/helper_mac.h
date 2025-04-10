#pragma once

#include "helper_posix.h"

// Mac commands
class Helper_mac : public Helper_posix
{
public:
    explicit Helper_mac(std::unique_ptr<IHelperBackend> backend);

    void setDnsScriptEnabled(bool bEnabled);
    void enableMacSpoofingOnBoot(bool bEnabled, const QString &interfaceName, const QString &macAddress);
    bool setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &entry);
    void setIpv6Enabled(bool bEnabled);
    void deleteRoute(const QString &range, int mask, const QString &gateway);
};
