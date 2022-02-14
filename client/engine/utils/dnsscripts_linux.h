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
    DnsScripts_linux();
    bool isUseResolvConf(bool bForceLogging);
    bool lastIsResolvConf_;
};

#endif // DNS_SCRIPTS_LINUX_H
