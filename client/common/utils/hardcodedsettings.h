#ifndef HARDCODEDSETTINGS_H
#define HARDCODEDSETTINGS_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include "version/appversion.h"
#include "utils/simplecrypt.h"

class HardcodedSettings
{
public:

    static HardcodedSettings &instance()
    {
        static HardcodedSettings s;
        return s;
    }

    QString serverDomain() const { return "windscribe.com"; }
    QString serverApiSubdomain() const {
        if (AppVersion::instance().isStaging()) return "api-staging";
        else return "api";
    }
    QString serverAssetsSubdomain() const { return "assets"; }
    QString serverTunnelTestSubdomain() const { return "checkip"; }
    QString serverUrl() const { return "www.windscribe.com"; }
    QString serverSharedKey() const { return "952b4412f002315aa50751032fcaab03"; }

    const QStringList openDns() const;
    const QStringList googleDns() const;
    const QStringList cloudflareDns() const;
    const QStringList controldDns() const;

    const QStringList apiIps() const { return apiIps_; }
    QString emergencyUsername() const { return emergencyUsername_; }
    QString emergencyPassword() const { return emergencyPassword_; }
    const QStringList emergencyIps() const { return emergencyIps_; }

    QString generateDomain();

private:
    HardcodedSettings();

    QStringList apiIps_;
    QString emergencyUsername_;
    QString emergencyPassword_;
    QStringList emergencyIps_;
    QByteArray passwordForRandomDomain_;

    SimpleCrypt simpleCrypt_;
    QStringList readArrayFromIni(const QSettings &settings, const QString &key, const QString &value, bool bWithDescrypt);
};

#endif // HARDCODEDSETTINGS_H
