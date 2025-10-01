#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

class HardcodedSettings
{
public:

    static HardcodedSettings &instance()
    {
        static HardcodedSettings s;
        return s;
    }

    QString windscribeHost() const { return "windscribe.com"; }
    QString windscribeServerUrl() const { return "www." + windscribeHost(); }

    const QStringList openDns() const;
    const QStringList googleDns() const;
    const QStringList cloudflareDns() const;
    const QStringList controldDns() const;

private:
    HardcodedSettings();
};
