#pragma once

#include "helper_posix.h"
#include <xpc/xpc.h>

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
    bool enableMacSpoofingOnBoot(bool bEnable, const QString &interfaceName, const QString &macAddress);
    bool setDnsOfDynamicStoreEntry(const QString &ipAddress, const QString &dynEnties);

    QString getInterfaceSsid(const QString &interfaceName);

protected:
    void doDisconnectAndReconnect() override;
    bool runCommand(int cmdId, const std::string &data, CMD_ANSWER &answer) override;

private:
    xpc_connection_t connection_;
};

