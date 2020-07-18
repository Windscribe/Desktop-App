#ifndef HARDCODEDSETTINGS_H
#define HARDCODEDSETTINGS_H

#include <QString>
#include <QStringList>
#include "Utils/simplecrypt.h"

class HardcodedSettings
{
public:

    static HardcodedSettings &instance()
    {
        static HardcodedSettings s;
        return s;
    }

    QString serverApiUrl() { return serverApiUrl_; }
    QString serverUrl() { return serverUrl_; }
    QString serverSharedKey() { return serverSharedKey_; }
    QStringList customDns() { return customDns_; }
    QStringList googleDns();
    QStringList cloudflareDns();
    QStringList apiIps() { return apiIps_; }

    QString emergencyUsername() { return emergencyUsername_; }
    QString emergencyPassword() { return emergencyPassword_; }
    QStringList emergencyIps() { return emergencyIps_; }

    QString generateRandomDomain(const QString &prefix);

private:
    HardcodedSettings();

    QString serverApiUrl_;
    QString serverUrl_;
    QString serverSharedKey_;
    QStringList customDns_;
    QStringList apiIps_;
    QString emergencyUsername_;
    QString emergencyPassword_;
    QStringList emergencyIps_;
    QByteArray passwordForRandomDomain_;

    SimpleCrypt simpleCrypt_;
};

#endif // HARDCODEDSETTINGS_H
