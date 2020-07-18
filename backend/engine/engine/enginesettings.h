#ifndef ENGINESETTINGS_H
#define ENGINESETTINGS_H

#include <QString>
#include "engine/types/protocoltype.h"
#include "engine/types/types.h"
#include "engine/types/connectionsettings.h"
#include "engine/types/dnsresolutionsettings.h"
#include "engine/proxy/proxysettings.h"
#include "ipc/command.h"
#include "ipc/generated_proto/types.pb.h"

class EngineSettings
{
public:
    EngineSettings();
    EngineSettings(const ProtoTypes::EngineSettings &s);

    void saveToSettings();
    void loadFromSettings();

    //IPC::Command *transformToProtoBufCommand(unsigned int cmdUid) const;

    const ProtoTypes::EngineSettings &getProtoBufEngineSettings() const;


    bool isEqual(const ProtoTypes::EngineSettings &s) const;

    QString language() const;
    bool isBetaChannel() const;
    bool isIgnoreSslErrors() const;
    bool isCloseTcpSockets() const;
    bool isAllowLanTraffic() const;
    const ProtoTypes::FirewallSettings &firewallSettings() const;
    ConnectionSettings connectionSettings() const;
    DnsResolutionSettings dnsResolutionSettings() const;
    ProxySettings proxySettings() const;
    DNS_POLICY_TYPE getDnsPolicy() const;
    ProtoTypes::MacAddrSpoofing getMacAddrSpoofing() const;
    ProtoTypes::PacketSize getPacketSize() const;

    bool isUseWintun() const;
    QString getCustomOvpnConfigsPath() const;

    bool isKeepAliveEnabled() const;

    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void setPacketSize(const ProtoTypes::PacketSize &packetSize);

private:
    ProtoTypes::EngineSettings engineSettings_;

    void loadFromVersion1(QSettings &settings);
};

#endif // ENGINESETTINGS_H
