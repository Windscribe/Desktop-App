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
    void enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress);
    QStringList getActiveNetworkInterfaces();
    bool setKeychainUsernamePassword(const QString &username, const QString &password);
    bool setKextPath(const QString &kextPath);
    bool setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &dynEnties);

private:
    bool setKeychainUsernamePasswordImpl(const QString &username, const QString &password, bool *bExecuted);
};

#endif // HELPER_MAC_H
