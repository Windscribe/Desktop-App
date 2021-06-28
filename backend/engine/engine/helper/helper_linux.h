#ifndef HELPER_LINUX_H
#define HELPER_LINUX_H

#include "helper_posix.h"

class Helper_linux : public Helper_posix
{
    Q_OBJECT
public:
    explicit Helper_linux(QObject *parent = 0);
    ~Helper_linux() override;

    // Common functions
    void startInstallHelper() override;
    bool reinstallHelper() override;
    QString getHelperVersion() override;

    bool setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress) override;
};

#endif // HELPER_LINUX_H
