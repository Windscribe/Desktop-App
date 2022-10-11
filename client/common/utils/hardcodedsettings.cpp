#include "utils/hardcodedsettings.h"
#include "utils/utils.h"
#include "utils/logger.h"
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

const QStringList HardcodedSettings::controldDns() const
{
    return QStringList() << "76.76.2.0" << "76.76.10.0";
}

QString HardcodedSettings::generateDomain()
{
    int randomNum = Utils::generateIntegerRandom(1, 3); // random number from 1 to 3
    QDateTime dt = QDateTime::currentDateTime();
    QByteArray str = passwordForRandomDomain_;
    str += QByteArray::number(randomNum);
    str += QByteArray::number(dt.date().month());
    str += QByteArray::number(dt.date().year());
    QByteArray code = QCryptographicHash::hash(str, QCryptographicHash::Sha1);
    QString result = code.toHex() + ".com";
    return result;
}

HardcodedSettings::HardcodedSettings() : simpleCrypt_(0x1272A4A3FE1A3DBA)
{
    // If the secrets.ini doesn't exist (open source users) then the strings will be empty
    // this is the same as having default empty strings in a dummy file
    QSettings secrets(":hardcodedsecrets.ini", QSettings::IniFormat);

    dynamicDomainsUrls_ = readArrayFromIni(secrets, "dynamicDomainsUrls", "url", true);
    if (dynamicDomainsUrls_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain dynamicDomainsUrls";

    dynamicDomains_ = readArrayFromIni(secrets, "dynamicDomains", "domain", true);
    if (dynamicDomains_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain dynamicDomains";

    serverDomains_ = readArrayFromIni(secrets, "hardcodedDomains", "domain", true);
    if (serverDomains_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain hardcodedDomains";

    apiIps_ = readArrayFromIni(secrets, "apiIps", "ip", true);
    if (apiIps_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain apiIps";

    emergencyUsername_ = simpleCrypt_.decryptToString(secrets.value("emergency/username").toString());
    emergencyPassword_ = simpleCrypt_.decryptToString(secrets.value("emergency/password").toString());
    if (emergencyUsername_.isEmpty() || emergencyPassword_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain emergency username/password";

    emergencyIps_ = readArrayFromIni(secrets, "emergencyIps", "ip", true);
    if (emergencyIps_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain emergencyIps";

    passwordForRandomDomain_ = simpleCrypt_.decryptToString(secrets.value("emergency/passwordForDomain").toString()).toStdString().c_str();
    if (passwordForRandomDomain_.isEmpty())
        qCDebug(LOG_BASIC) << "Warning: the hardcodedsecrets.ini file does not contain passwordForRandomDomain_";
}

QStringList HardcodedSettings::readArrayFromIni(const QSettings &settings, const QString &key, const QString &value, bool bWithDescrypt)
{
    QStringList ret;
    int ind = 1;

    while (true) {
        QVariant v = settings.value(key + "/" + value + QString::number(ind));
        if (v.isValid()) {
            if (bWithDescrypt)
                ret << simpleCrypt_.decryptToString(v.toString());
            else
                ret << v.toString();
            ind++;
        } else  {
            break;
        }
    };

    return ret;
}
