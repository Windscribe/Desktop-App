#ifndef WIREGUARDCUSTOMCONFIG_H
#define WIREGUARDCUSTOMCONFIG_H

#include "icustomconfig.h"
#include "engine/types/wireguardconfig.h"
#include <QSharedPointer>

namespace customconfigs {

class WireguardCustomConfig : public ICustomConfig
{
public:
    CUSTOM_CONFIG_TYPE type() const override;
    QString name() const override;      // use for show in the GUI, basically filename without extension
    QString filename() const override;
    QStringList hostnames() const override;      // hostnames/ips

    bool isCorrect() const override;
    QString getErrorForIncorrect() const override;

    QSharedPointer<WireGuardConfig> getWireGuardConfig(const QString &endpointIp) const;
    uint getEndpointPort() const { return endpointPortNumber_; }

    static ICustomConfig *makeFromFile(const QString &filepath);

private:
    void loadFromFile(const QString &filepath);
    void validate();

    QString errMessage_;
    QString name_;
    QString filename_;

    QString privateKey_;
    QString ipAddress_;
    QString dnsAddress_;
    QString publicKey_;
    QString presharedKey_;
    QString allowedIps_;
    QString endpointHostname_;
    QString endpointPort_;
    uint endpointPortNumber_;
};

} //namespace customconfigs

#endif // WIREGUARDCUSTOMCONFIG_H
