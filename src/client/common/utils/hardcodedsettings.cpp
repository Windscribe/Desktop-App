#include "hardcodedsettings.h"

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
    return QStringList() << "76.76.2.22" << "76.76.10.0";
}

HardcodedSettings::HardcodedSettings()
{
}

