#ifndef HELPER_MAC_H
#define HELPER_MAC_H

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

    bool setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress) override;

    // Mac specific functions
    bool setMacAddress(const QString &interface, const QString &macAddress, bool robustMethod);
    bool enableMacSpoofingOnBoot(bool bEnable, const QString &interfaceName, const QString &macAddress, bool robustMethod);
    bool setKeychainUsernamePassword(const QString &username, const QString &password);
    bool setKextPath(const QString &kextPath);
    bool setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &dynEnties);
    bool setIpv6Enabled(bool bEnabled);

private:
    bool setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted);
};

#endif // HELPER_MAC_H
