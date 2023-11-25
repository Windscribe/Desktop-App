#pragma once

#include "helper_posix.h"

class Helper_mac : public Helper_posix
{
    Q_OBJECT
public:
    explicit Helper_mac(QObject *parent = 0);
    ~Helper_mac() override;

    // Common functions
    void startInstallHelper() override;
    bool reinstallHelper() override;
    QString getHelperVersion() override;

    // Mac specific functions
    bool setMacAddress(const QString &interface, const QString &macAddress);
    bool enableMacSpoofingOnBoot(bool bEnable, const QString &interfaceName, const QString &macAddress);
    bool setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &dynEnties);
    bool setIpv6Enabled(bool bEnabled);
};

