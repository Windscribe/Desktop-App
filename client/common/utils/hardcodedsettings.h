#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

#include "version/appversion.h"

class HardcodedSettings
{
public:

    static HardcodedSettings &instance()
    {
        static HardcodedSettings s;
        return s;
    }

    QString primaryServerDomain() const { return "windscribe.com"; }

    QString serverApiSubdomain() const;
    QString serverAssetsSubdomain() const;
    QString serverTunnelTestSubdomain() const { return "checkip"; }
    QString serverUrl() const;
    QString serverSharedKey() const { return "952b4412f002315aa50751032fcaab03"; }

    const QStringList openDns() const;
    const QStringList googleDns() const;
    const QStringList cloudflareDns() const;
    const QStringList controldDns() const;

    QString generateDomain();

private:
    HardcodedSettings();
};

inline QString HardcodedSettings::serverApiSubdomain() const
{
    if (AppVersion::instance().isStaging())
        return "api-staging";

    return "api";
}

inline QString HardcodedSettings::serverAssetsSubdomain() const
{
    if (AppVersion::instance().isStaging())
        return "assets-staging";

    return "assets";
}

inline QString HardcodedSettings::serverUrl() const
{
    if (AppVersion::instance().isStaging())
        return "www-staging.windscribe.com";

    return "www.windscribe.com";
}
