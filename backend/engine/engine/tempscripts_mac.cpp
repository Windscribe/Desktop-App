#include "tempscripts_mac.h"
#include <QCoreApplication>
#include <QFile>

QString TempScripts_mac::dnsScriptPath()
{
    if (dnsScriptPath_.isEmpty() || !QFile::exists(dnsScriptPath_))
    {
        QString strTemp = QString::fromLocal8Bit(getenv("TMPDIR"));

        dnsScriptPath_ = strTemp + "windscribedns.sh";

        QString srcDns = QCoreApplication::applicationDirPath() + "/../Resources/dns.sh";
        QFile::remove(dnsScriptPath_);
        QFile::copy(srcDns, dnsScriptPath_);
    }
    return dnsScriptPath_;
}

QString TempScripts_mac::removeHostsScriptPath()
{
    if (removeHostsScriptPath_.isEmpty() || !QFile::exists(removeHostsScriptPath_))
    {
        QString strTemp = QString::fromLocal8Bit(getenv("TMPDIR"));

        removeHostsScriptPath_ = strTemp + "windscriberemovehosts.sh";
        QString src = QCoreApplication::applicationDirPath() + "/manage-etc-hosts.sh";
        QFile::remove(removeHostsScriptPath_);
        QFile::copy(src, removeHostsScriptPath_);
    }
    return removeHostsScriptPath_;
}

TempScripts_mac::TempScripts_mac()
{
}

TempScripts_mac::~TempScripts_mac()
{
    {
        if (!dnsScriptPath_.isEmpty())
        {
            QFile file(dnsScriptPath_);
            file.remove();
        }
    }
    {
        if (!removeHostsScriptPath_.isEmpty())
        {
            QFile file(removeHostsScriptPath_);
            file.remove();
        }
    }
}
