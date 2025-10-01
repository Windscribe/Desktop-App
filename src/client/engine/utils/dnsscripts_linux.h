#pragma once

#include "types/enums.h"
#include <QString>

class IHelper;
class QWidget;

// Linux DNS-script management, depending on the installed utilities (resolvconf, systemd-resolved, ...)
class DnsScripts_linux
{
public:
    enum SCRIPT_TYPE { SYSTEMD_RESOLVED, RESOLV_CONF, NETWORK_MANAGER };

    static DnsScripts_linux &instance()
    {
        static DnsScripts_linux s;
        return s;
    }

    SCRIPT_TYPE dnsManager();
    void setDnsManager(DNS_MANAGER_TYPE d);

private:
    DnsScripts_linux();

    DNS_MANAGER_TYPE dnsManager_;
    SCRIPT_TYPE detectScript();
    QString getSymlink(const QString &path);
};
