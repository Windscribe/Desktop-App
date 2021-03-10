#include "hardcodedsettings.h"
#include "utils/utils.h"
#include "version/windscribe_version.h"
#include <QDateTime>
#include <QStringList>
#include <QCryptographicHash>

const QStringList HardcodedSettings::googleDns() const
{
    return QStringList() << "8.8.8.8" << "8.8.4.4";
}

const QStringList HardcodedSettings::cloudflareDns() const
{
    return QStringList() << "1.1.1.1" << "1.0.0.1";
}

QString HardcodedSettings::generateRandomDomain(const QString &prefix)
{

    int randomNum = Utils::generateIntegerRandom(1, 3); // random number from 1 to 3
    //qCDebug(LOG_BASIC) << "Random number for generated domain:" << randomNum;
    QDateTime dt = QDateTime::currentDateTime();
    //qCDebug(LOG_BASIC) << "Year for generated domain:" << dt.date().year();
    //qCDebug(LOG_BASIC) << "Month for generated domain:" << dt.date().month();

    QByteArray str = passwordForRandomDomain_;    // "giveMEsomePACKETZ9!"
    str += QString::number(randomNum);
    str += QString::number(dt.date().month());
    str += QString::number(dt.date().year());
    QByteArray code = QCryptographicHash::hash(str, QCryptographicHash::Sha1);
    QString result = prefix + code.toHex() + ".com";
    return result;
}

HardcodedSettings::HardcodedSettings() : simpleCrypt_(0x1272A4A3FE1A3DBA)
{
    // strings crypted for security (can't see from exe with text editor)
#ifdef WINDSCRIBE_IS_STAGING
    serverApiUrl_ = simpleCrypt_.decryptToString(QString("AwKT27It/jNsDcOf4nW4eydCkcK8MfEnPEyTgPlopg==")); // api-staging.windscribe.com
#else
    serverApiUrl_ = simpleCrypt_.decryptToString(QString("AwKTH1LNHtOP6jlqFJlZj5TkOyhRwA4=")); // api.windscribe.com
#endif
    serverUrl_ = simpleCrypt_.decryptToString(QString("AwKTVtZfi1gEYbLhnxLSBB9vsKPaS4U=")); // www.windscribe.com
    serverSharedKey_ = simpleCrypt_.decryptToString(QString("AwKTgtgfiR8PKaergxuIHFx99v6FGowYXXrx/NUZ3BsIe6Ouhw==")); // 952b4412f002315aa50751032fcaab03
    serverTunnelTestUrl_ = simpleCrypt_.decryptToString(QString("AwJKLI8S2RgJcKPu2lOZU0Uk/bLBXZsRAH2q")); // checkip.windscribe.com

    customDns_ << simpleCrypt_.decryptToString(QString("AwKTYSPvfOC8mBUGLuJz+bmZEQ==")); // 208.67.222.222
    customDns_ << simpleCrypt_.decryptToString(QString("AwKTVIxA008TN7qpgU3eVBQ0vg==")); // 208.67.220.220

    apiIps_ << simpleCrypt_.decryptToString(QString("AwKTSSLtfeG9nh0XI+x67rKXGw==")); // 138.197.150.76
    apiIps_ << simpleCrypt_.decryptToString(QString("AwKTK/s0pDllRsrF8T6oPGBDzME=")); // 139.162.150.150

    emergencyUsername_ = simpleCrypt_.decryptToString(QString("AwKTwHT9N/3rilMcb/M1")); // windscribe
    emergencyPassword_ = simpleCrypt_.decryptToString(QString("AwKTi1r8OvG1zC9AaA==")); // Xeo6kYR2

    emergencyIps_ << simpleCrypt_.decryptToString(QString("AwKTFVSbC5fL6GthVZoPmcXmZW0=")); // 138.197.162.195
    emergencyIps_ << simpleCrypt_.decryptToString(QString("AwKT4ArFVsaauTA8CMdSwJy/Nz4=")); // 104.131.166.124

    passwordForRandomDomain_ = simpleCrypt_.decryptToString(QString("AwKTgZYPxRcAX6DumwjOOglYqdGfO6Ek")).toStdString().c_str(); // giveMEsomePACKETZ9!
}
