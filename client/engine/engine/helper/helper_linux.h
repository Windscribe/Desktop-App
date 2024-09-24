#pragma once

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

    // linux specific
    std::optional<bool> installUpdate(const QString& package) const;
    bool setDnsLeakProtectEnabled(bool bEnabled);
    bool resetMacAddresses(const QString &ignoreNetwork = "");
};
