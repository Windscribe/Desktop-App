#ifndef TYPES_ENGINESETTINGS_H
#define TYPES_ENGINESETTINGS_H

#include <QString>
#include <QSharedDataPointer>
#include <QSharedData>
#include "types/protocoltype.h"
#include "types/enums.h"
#include "types/connectionsettings.h"
#include "types/dnsresolutionsettings.h"
#include "types/proxysettings.h"
#include "types/firewallsettings.h"
#include "types/packetsize.h"
#include "types/macaddrspoofing.h"
#include "types/dnswhileconnectedinfo.h"
#include "utils/simplecrypt.h"

namespace types {

struct EngineSettingsData : public QSharedData
{
    EngineSettingsData() :
        language("en"),
        updateChannel(UPDATE_CHANNEL_RELEASE),
        isIgnoreSslErrors(false),
        isCloseTcpSockets(true),
        isAllowLanTraffic(false),
        dnsPolicy(DNS_TYPE_OS_DEFAULT),
        tapAdapter(WINTUN_ADAPTER),
        isKeepAliveEnabled(false),
        dnsManager(DNS_MANAGER_AUTOMATIC)
    {}

    QString language;
    UPDATE_CHANNEL updateChannel;
    bool isIgnoreSslErrors;
    bool isCloseTcpSockets;
    bool isAllowLanTraffic;
    types::FirewallSettings firewallSettings;
    types::ConnectionSettings connectionSettings;
    types::DnsResolutionSettings dnsResolutionSettings;
    types::ProxySettings proxySettings;
    types::PacketSize packetSize;
    types::MacAddrSpoofing macAddrSpoofing;
    DNS_POLICY_TYPE dnsPolicy;
    TAP_ADAPTER_TYPE tapAdapter;
    QString customOvpnConfigsPath;
    bool isKeepAliveEnabled;
    types::DnsWhileConnectedInfo dnsWhileConnectedInfo;
    DNS_MANAGER_TYPE dnsManager;
};


// implicitly shared class EngineSettings
class EngineSettings
{
public:
    explicit EngineSettings();

    void saveToSettings();
    void loadFromSettings();

    //bool isEqual(const ProtoTypes::EngineSettings &s) const;

    QString language() const;
    bool isIgnoreSslErrors() const;
    bool isCloseTcpSockets() const;
    bool isAllowLanTraffic() const;
    const types::FirewallSettings &firewallSettings() const;
    const types::ConnectionSettings &connectionSettings() const;
    const types::DnsResolutionSettings &dnsResolutionSettings() const;
    const types::ProxySettings &proxySettings() const;
    DNS_POLICY_TYPE getDnsPolicy() const;
    DNS_MANAGER_TYPE getDnsManager() const;
    const types::MacAddrSpoofing &getMacAddrSpoofing() const;
    const types::PacketSize &getPacketSize() const;
    UPDATE_CHANNEL getUpdateChannel() const;
    const types::DnsWhileConnectedInfo &getDnsWhileConnectedInfo() const;
    bool isUseWintun() const;
    QString getCustomOvpnConfigsPath() const;
    bool isKeepAliveEnabled() const;

    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing);
    void setPacketSize(const types::PacketSize &packetSize);

    bool operator==(const EngineSettings &other) const;
    bool operator!=(const EngineSettings &other) const;


private:
    QSharedDataPointer<EngineSettingsData> d;

    /*ProtoTypes::EngineSettings engineSettings_;

    SimpleCrypt simpleCrypt_;

    void loadFromVersion1();*/

#if defined(Q_OS_LINUX)
    void repairEngineSettings();
#endif
};

} // types namespace

#endif // TYPES_ENGINESETTINGS_H
