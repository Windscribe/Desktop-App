#pragma once

#include "types/protocol.h"

class MakeOVPNFile
{
public:
    MakeOVPNFile();
    virtual ~MakeOVPNFile();

    bool generate(const QString &ovpnData, const QString &ip, types::Protocol protocol, uint port,
                  uint portForStunnelOrWStunnel, int mss, const QString &defaultGateway, const QString &openVpnX509,
                  const QString &customDns, bool isAntiCensorship, const QString &awgPreset);
    QString config() { return config_; }

    // Shared with the custom-config prepare path so the two DNS-override copies can't drift.
    static void appendDnsOverride(QString &config, const QString &dnsServer);

private:
    QString config_;
};
