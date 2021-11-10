#ifndef HARDCODEDSETTINGS_H
#define HARDCODEDSETTINGS_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include "utils/simplecrypt.h"

class HardcodedSettings
{
public:

    static HardcodedSettings &instance()
    {
        static HardcodedSettings s;
        return s;
    }

    QString serverApiUrl() const { return serverApiUrl_; }
    QString serverUrl() const { return serverUrl_; }
    QString serverSharedKey() const { return serverSharedKey_; }
    QString serverTunnelTestUrl() const { return serverTunnelTestUrl_; }

    const QStringList openDns() const;
    const QStringList googleDns() const;
    const QStringList cloudflareDns() const;
    const QStringList apiIps() const { return apiIps_; }

    QString emergencyUsername() const { return emergencyUsername_; }
    QString emergencyPassword() const { return emergencyPassword_; }
    const QStringList emergencyIps() const { return emergencyIps_; }

    QString windowsCertName() const { return windowsCertName_; }
    QString macCertName() const { return macCertName_; }

    QString generateDomain(const QString &prefix);

private:
    HardcodedSettings();

    QString serverApiUrl_;
    QString serverUrl_;
    QString serverSharedKey_;
    QString serverTunnelTestUrl_;
    QStringList apiIps_;
    QString emergencyUsername_;
    QString emergencyPassword_;
    QStringList emergencyIps_;
    QByteArray passwordForRandomDomain_;
    QString windowsCertName_;
    QString macCertName_;

    SimpleCrypt simpleCrypt_;

    QStringList readArrayFromIni(const QSettings &settings, const QString &key, const QString &value, bool bWithDescrypt);
};

#endif // HARDCODEDSETTINGS_H