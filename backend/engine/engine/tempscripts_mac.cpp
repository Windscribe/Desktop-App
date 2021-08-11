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
}
