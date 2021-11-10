#include "hardcodedsettings.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "version/windscribe_version.h"
#include <QDateTime>
#include <QStringList>
#include <QCryptographicHash>

const QStringList HardcodedSettings::openDns() const
{
    return QStringList() << "208.67.222.222" << "208.67.220.220";
}

const QStringList HardcodedSettings::googleDns() const
{
    return QStringList() << "8.8.8.8" << "8.8.4.4";
}

const QStringList HardcodedSettings::cloudflareDns() const
{
    return QStringList() << "1.1.1.1" << "1.0.0.1";
}

QString HardcodedSettings::generateDomain(const QString &prefix)
{
    int randomNum = Utils::generateIntegerRandom(1, 3); // random number from 1 to 3
    QDateTime dt = QDateTime::currentDateTime();
    QByteArray str = passwordForRandomDomain_;
    str += QString::number(randomNum);
    str += QString::number(dt.date().month());
    str += QString::number(dt.date().year());
    QByteArray code = QCryptographicHash::hash(str, QCryptographicHash::Sha1);
    QString result = prefix + code.toHex() + ".com";
    return result;
}

HardcodedSettings::HardcodedSettings() : simpleCrypt_(0x1272A4A3FE1A3DBA)
{
    QSettings settings(":/common/utils/hardcodedsettings.ini", QSettings::IniFormat);

    // some INI strings are crypted
#ifdef WINDSCRIBE_IS_STAGING
    serverApiUrl_ = settings.value("basic/serverApiUrlStaging").toString();
#else
    serverApiUrl_ = settings.value("basic/serverApiUrl").toString();
#endif
    serverUrl_ = settings.value("basic/serverUrl").toString();
    serverSharedKey_ = settings.value("basic/serverSharedKey").toString();
    serverTunnelTestUrl_ = settings.value("basic/serverTunnelTestUrl").toString();

    windowsCertName_ = settings.value("certificateName/windows").toString();
    macCertName_ = settings.value("certificateName/mac").toString();

    apiIps_ = readArrayFromIni(settings, "apiIps", "ip", true);

    emergencyUsername_ = simpleCrypt_.decryptToString(settings.value("emergency/username").toString());
    emergencyPassword_ = simpleCrypt_.decryptToString(settings.value("emergency/password").toString());

    emergencyIps_ = readArrayFromIni(settings, "emergencyIps", "ip", true);

    passwordForRandomDomain_ = simpleCrypt_.decryptToString(settings.value("emergency/passwordForDomain").toString()).toStdString().c_str();
}

QStringList HardcodedSettings::readArrayFromIni(const QSettings &settings, const QString &key, const QString &value, bool bWithDescrypt)
{
    QStringList ret;
    int ind = 1;

    while (true)
    {
        QVariant v = settings.value(key + "/" + value + QString::number(ind));
        if (v.isValid())
        {
            if (bWithDescrypt)
            {
                ret << simpleCrypt_.decryptToString(v.toString());
            }
            else
            {
                ret << v.toString();
            }
            ind++;
        }
        else
        {
            break;
        }
    };

    return ret;
}