#pragma once

#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>

#include "engine/customconfigs/ovpncustomconfig.h"
#include "engine/customconfigs/wireguardcustomconfig.h"
#include "engine/locationsmodel/customconfiglocationinfo.h"
#include "types/locationid.h"

// Shared test fixtures: a custom-config body is written to a temp file, parsed through the real
// makeFromFile/isCorrect pipeline, and wrapped in a CustomConfigLocationInfo. Used by both the
// locationsmodel and connectionmanager test suites so the two never drift on config shape.
namespace customconfig_fixtures {

inline QSharedPointer<const customconfigs::ICustomConfig> loadOvpnConfig(const QString &body)
{
    QTemporaryFile tmp(QDir::tempPath() + "/ovpntest_XXXXXX.ovpn");
    if (!tmp.open()) {
        return nullptr;
    }
    QTextStream ts(&tmp);
    ts << body;
    ts.flush();
    const QString path = tmp.fileName();
    tmp.close();
    QSharedPointer<const customconfigs::ICustomConfig> config(customconfigs::OvpnCustomConfig::makeFromFile(path));
    return (config && config->isCorrect()) ? config : nullptr;
}

inline QSharedPointer<locationsmodel::CustomConfigLocationInfo> makeOvpnCustomConfigLocationInfo(
    const QString &body = "dev tun\nauth-user-pass\nremote 1.2.3.4 443\n")
{
    auto config = loadOvpnConfig(body);
    if (!config) {
        return nullptr;
    }
    return QSharedPointer<locationsmodel::CustomConfigLocationInfo>::create(
        LocationID::createCustomConfigLocationId(config->filename()), config);
}

inline QSharedPointer<locationsmodel::CustomConfigLocationInfo> makeWireGuardCustomConfigLocationInfo(
    const QString &endpoint = "1.2.3.4:51820")
{
    QTemporaryFile tmp(QDir::tempPath() + "/wgtest_XXXXXX.conf");
    if (!tmp.open()) {
        return nullptr;
    }
    QTextStream ts(&tmp);
    ts << "[Interface]\n"
          "PrivateKey = YAnz5TF+lXXJte14tji3zlMNq+hd2rYUIgJBgB3fBmk=\n"
          "Address = 10.0.0.2/32\n"
          "DNS = 10.255.255.1\n"
          "\n"
          "[Peer]\n"
          "PublicKey = xTIBA5rboUvnH4htodjb6e697QjLERt1NAB4mZqp8Dg=\n"
          "AllowedIPs = 0.0.0.0/0\n"
          "Endpoint = " << endpoint << "\n";
    ts.flush();
    const QString path = tmp.fileName();
    tmp.close();
    QSharedPointer<const customconfigs::ICustomConfig> config(customconfigs::WireguardCustomConfig::makeFromFile(path));
    if (config.isNull() || !config->isCorrect()) {
        return nullptr;
    }
    return QSharedPointer<locationsmodel::CustomConfigLocationInfo>::create(
        LocationID::createCustomConfigLocationId(config->filename()), config);
}

} // namespace customconfig_fixtures
