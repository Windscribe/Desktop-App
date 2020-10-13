#include "wireguardcustomconfig.h"

#include <QFileInfo>

namespace customconfigs {

CUSTOM_CONFIG_TYPE WireguardCustomConfig::type() const
{
    return CUSTOM_CONFIG_WIREGUARD;
}

QString WireguardCustomConfig::name() const
{
    return name_;
}

QString WireguardCustomConfig::filename() const
{
    return filename_;
}

QStringList WireguardCustomConfig::hostnames() const
{
    //todo
    return QStringList();
}

bool WireguardCustomConfig::isCorrect() const
{
    //todo
    return true;
}

QString WireguardCustomConfig::getErrorForIncorrect() const
{
    //todo
    return "";
}

ICustomConfig *WireguardCustomConfig::makeFromFile(const QString &filepath)
{
    //todo
    WireguardCustomConfig *config = new WireguardCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();
    return config;
}


} //namespace customconfigs

