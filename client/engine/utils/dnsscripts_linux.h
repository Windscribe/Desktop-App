#ifndef DNS_SCRIPTS_LINUX_H
#define DNS_SCRIPTS_LINUX_H

#include <QString>


class IHelper;
class QWidget;

// Linux DNS-script management, depending on the installed utilities (resolvconf, systemd-resolved, ...)
class DnsScripts_linux
{
public:
    static DnsScripts_linux &instance()
    {
        static DnsScripts_linux s;
        return s;
    }

    QString scriptPath();

private:

    enum SCRIPT_TYPE { SYSTEMD_RESOLVED, RESOLV_CONF, NETWORK_MANAGER };
    SCRIPT_TYPE detectScript();
};

#endif // DNS_SCRIPTS_LINUX_H
