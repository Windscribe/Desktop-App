#pragma once

#include "types/protocol.h"

class MakeOVPNFile
{
public:
    MakeOVPNFile();
    virtual ~MakeOVPNFile();

    bool generate(const QString &ovpnData, const QString &ip, types::Protocol protocol, uint port,
                  uint portForStunnelOrWStunnel, int mss, const QString &defaultGateway, const QString &openVpnX509, const QString &customDns);
    QString config() { return config_; }

private:
    QString config_;
};
