#ifndef DNS_SCRIPTS_LINUX_H
#define DNS_SCRIPTS_LINUX_H

#include <QString>
#include "utils/protobuf_includes.h"


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

    void setDnsManager(ProtoTypes::DnsManagerType d);

private:
    DnsScripts_linux();

    enum SCRIPT_TYPE { SYSTEMD_RESOLVED, RESOLV_CONF, NETWORK_MANAGER };
    ProtoTypes::DnsManagerType dnsManager_;
    SCRIPT_TYPE detectScript();
};

#endif // DNS_SCRIPTS_LINUX_H
