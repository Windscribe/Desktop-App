#include "hardcodedsettings.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QStringList>

#include "utils/utils.h"
#include "dga_library.h"
#include "dga_parameters.h"

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
    DgaLibrary dga;
    if (dga.load()) {
        QByteArray passwordForRandomDomain = dga.getParameter(PAR_PASSWORD_FOR_RANDOM_DOMAIN).toStdString().c_str();
        int randomNum = Utils::generateIntegerRandom(1, 3); // random number from 1 to 3
        QDateTime dt = QDateTime::currentDateTime();
        QByteArray str = passwordForRandomDomain;
        str += QByteArray::number(randomNum);
        str += QByteArray::number(dt.date().month());
        str += QByteArray::number(dt.date().year());
        QByteArray code = QCryptographicHash::hash(str, QCryptographicHash::Sha1);
        QString result = code.toHex() + ".com";
        return result;
    }
    return "";
}

HardcodedSettings::HardcodedSettings()
{
}

