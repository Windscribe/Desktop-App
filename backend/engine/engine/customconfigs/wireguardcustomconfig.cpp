#include "wireguardcustomconfig.h"
#include "utils/logger.h"

#include <QFileInfo>
#include <QSettings>

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
    QStringList list;
    if (!endpointHostname_.isEmpty())
        list << endpointHostname_;
    return list;
}

bool WireguardCustomConfig::isCorrect() const
{
    return errMessage_.isEmpty();
}

QString WireguardCustomConfig::getErrorForIncorrect() const
{
    return errMessage_;
}

QSharedPointer<WireGuardConfig> WireguardCustomConfig::getWireGuardConfig(const QString &endpointIp) const
{
    auto *config = new WireGuardConfig(privateKey_, ipAddress_, dnsAddress_, publicKey_,
                                       presharedKey_, endpointIp + endpointPort_, allowedIps_);
    return QSharedPointer<WireGuardConfig>(config);
}

// static
ICustomConfig *WireguardCustomConfig::makeFromFile(const QString &filepath)
{
    WireguardCustomConfig *config = new WireguardCustomConfig();
    QFileInfo fi(filepath);
    config->name_ = fi.completeBaseName();
    config->filename_ = fi.fileName();
    config->loadFromFile(filepath);  // here the config can change to incorrect
    config->validate();
    return config;
}

void WireguardCustomConfig::loadFromFile(const QString &filepath)
{
    QSettings file(filepath, QSettings::IniFormat);
    switch (file.status()) {
    default:
    case QSettings::AccessError:
        qDebug(LOG_CUSTOM_OVPN) << "Failed to open file" << filepath;
        errMessage_ = QObject::tr("Failed to open file");
        return;
    case QSettings::FormatError:
        qDebug(LOG_CUSTOM_OVPN) << "Failed to parse file" << filepath;
        errMessage_ = QObject::tr("Invalid config format");
        return;
    case QSettings::NoError:
        // File exists and format is correct.
        break;
    }
    auto groups = file.childGroups();
    if (groups.indexOf("Interface") < 0) {
        errMessage_ = QObject::tr("Missing \"Interface\" section");
        return;
    }
    file.beginGroup("Interface");
    privateKey_ = file.value("PrivateKey").toString();
    ipAddress_ = file.value("Address").toString();
    dnsAddress_ = file.value("DNS").toString();
    file.endGroup();

    if (groups.indexOf("Peer") < 0) {
        errMessage_ = QObject::tr("Missing \"Interface\" section");
        return;
    }
    file.beginGroup("Peer");
    publicKey_ = file.value("PublicKey").toString();
    presharedKey_ = file.value("PresharedKey").toString();
    allowedIps_ = file.value("AllowedIPs", "0.0.0.0/0").toString();
    QStringList endpointParts = file.value("Endpoint").toString().split(":");
    endpointHostname_ = endpointParts[0];
    endpointPort_.clear();
    endpointPortNumber_ = 0;
    if (endpointParts.size() > 1) {
        endpointPortNumber_ = endpointParts[1].toUInt();
        if (endpointPortNumber_ > 0)
            endpointPort_ = QString(":%1").arg(endpointPortNumber_);
    }
    file.endGroup();
}

void WireguardCustomConfig::validate()
{
    // If already incorrect, skip further validation.
    if (!errMessage_.isEmpty())
        return;

    // Some fields are required.
    if (privateKey_.isEmpty())
        errMessage_ = QObject::tr("Missing \"PrivateKey\" in the \"Interface\" section");
    else if (ipAddress_.isEmpty())
        errMessage_ = QObject::tr("Missing \"Address\" in the \"Interface\" section");
    else if (dnsAddress_.isEmpty())
        errMessage_ = QObject::tr("Missing \"DNS\" in the \"Interface\" section");
    else if (publicKey_.isEmpty())
        errMessage_ = QObject::tr("Missing \"PublicKey\" in the \"Peer\" section");
    else if (endpointHostname_.isEmpty())
        errMessage_ = QObject::tr("Missing \"Endpoint\" in the \"Peer\" section");
}

} //namespace customconfigs

