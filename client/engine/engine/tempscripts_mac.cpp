#include "tempscripts_mac.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>

#include "utils/logger.h"

QString TempScripts_mac::dnsScriptPath()
{
    QString srcDns = QCoreApplication::applicationDirPath() + "/../Resources/dns.sh";

    if (dnsScriptPath_.isEmpty() || !QFile::exists(dnsScriptPath_))
    {
        QString strTemp = QString::fromLocal8Bit(getenv("TMPDIR"));
        dnsScriptPath_ = strTemp + "windscribedns.sh";

        if (QFile::exists(dnsScriptPath_) && !QFile::remove(dnsScriptPath_))
        {
            qCDebug(LOG_CONNECTION) << "could not remove existing script file:" << dnsScriptPath_;
            return QString();
        }

        if (!QFile::copy(srcDns, dnsScriptPath_))
        {
            qCDebug(LOG_CONNECTION) << "could not copy bundled dns.sh to" << dnsScriptPath_;
            return QString();
        }
    }

    // Ensure the user has not modified the copied dns.sh file, as we will be asking Helper
    // to run it as root.
    QByteArray srcDnsHash = hashFile(srcDns);
    if (srcDnsHash.isEmpty())
    {
        qCDebug(LOG_CONNECTION) << "could not hash" << srcDns;
        return QString();
    }

    QByteArray targetDnsHash = hashFile(dnsScriptPath_);
    if (targetDnsHash.isEmpty())
    {
        qCDebug(LOG_CONNECTION) << "could not hash" << dnsScriptPath_;
        return QString();
    }

    if (targetDnsHash != srcDnsHash)
    {
        qCDebug(LOG_CONNECTION) << dnsScriptPath_ << "has been modified and cannot be run. It's content does not match the bundled dns.sh file.";
        return QString();
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

QByteArray TempScripts_mac::hashFile(const QString& file) const
{
    QByteArray result;

    QFile fi(file);
    if (fi.open(QIODeviceBase::ReadOnly))
    {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&fi)) {
            result = hash.result().toHex();
        }
    }

    return result;
}
