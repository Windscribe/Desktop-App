#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QString>

#include "types/apiresolutionsettings.h"
#include "types/connecteddnsinfo.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/firewallsettings.h"
#include "types/macaddrspoofing.h"
#include "types/packetsize.h"
#include "types/proxysettings.h"

class LegacyProtobufSupport;

namespace types {

struct EngineSettingsData : public QSharedData
{
    struct JsonInfo {
        const QString kApiResolutionSettingsProp = "apiResolutionSettings";
        const QString kConnectedDnsInfoProp = "connectedDnsInfo";
        const QString kConnectionSettingsProp = "connectionSettings";
        const QString kCustomOvpnConfigsPathProp = "customOvpnConfigsPath";
        const QString kDnsManagerProp = "dnsManager";
        const QString kDnsPolicyProp = "dnsPolicy";
        const QString kFirewallSettingsProp = "firewallSettings";
        const QString kIsAllowLanTrafficProp = "isAllowLanTraffic";
        const QString kIsAntiCensorshipProp = "isAntiCensorship";
        const QString kIsIgnoreSslErrorsProp = "isIgnoreSslErrors";
        const QString kIsKeepAliveEnabledProp = "isKeepAliveEnabled";
        const QString kIsTerminateSocketsProp = "isTerminateSockets";
        const QString kLanguageProp = "language";
        const QString kMacAddrSpoofingProp = "macAddrSpoofing";
        const QString kNetworkLastKnownGoodProtocolsProp = "networkLastKnownGoodProtocols";
        const QString kNetworkPreferredProtocolsProp = "networkPreferredProtocols";
        const QString kPacketSizeProp = "packetSize";
        const QString kProtocolProp = "protocol";
        const QString kProxySettingsProp = "proxySettings";
        const QString kTapAdapterProp = "tapAdapter";
        const QString kUpdateChannelProp = "updateChannel";
        const QString kValueProp = "value";
        const QString kVersionProp = "version";
    };

    EngineSettingsData() :
        updateChannel(UPDATE_CHANNEL_RELEASE),
        isIgnoreSslErrors(false),
        isTerminateSockets(true),
        isAntiCensorship(false),
        isAllowLanTraffic(false),
        dnsPolicy(DNS_TYPE_CLOUDFLARE),
        tapAdapter(WINTUN_ADAPTER),
        isKeepAliveEnabled(false),
        dnsManager(DNS_MANAGER_AUTOMATIC)
    {}

    UPDATE_CHANNEL updateChannel;
    bool isIgnoreSslErrors;
    bool isTerminateSockets;
    bool isAntiCensorship;
    bool isAllowLanTraffic;
    types::FirewallSettings firewallSettings;
    types::ConnectionSettings connectionSettings;
    types::ApiResolutionSettings apiResolutionSettings;
    types::ProxySettings proxySettings;
    types::PacketSize packetSize;
    types::MacAddrSpoofing macAddrSpoofing;
    DNS_POLICY_TYPE dnsPolicy;
    TAP_ADAPTER_TYPE tapAdapter;
    QString customOvpnConfigsPath;
    bool isKeepAliveEnabled;
    types::ConnectedDnsInfo connectedDnsInfo;
    DNS_MANAGER_TYPE dnsManager;
    QMap<QString, types::ConnectionSettings> networkPreferredProtocols;
    QMap<QString, std::pair<types::Protocol, uint>> networkLastKnownGoodProtocols;
    QString language;

    JsonInfo jsonInfo;

    QJsonObject toJson() const;
};


// implicitly shared class EngineSettings
class EngineSettings
{
public:
    explicit EngineSettings();
    EngineSettings(const QJsonObject& json);

    void saveToSettings();
    bool loadFromSettings();

    //bool isEqual(const ProtoTypes::EngineSettings &s) const;

    QString language() const;
    void setLanguage(const QString &lang);

    bool isIgnoreSslErrors() const;
    void setIsIgnoreSslErrors(bool ignore);
    bool isTerminateSockets() const;
    void setIsTerminateSockets(bool close);
    bool isAntiCensorship() const;
    void setIsAntiCensorship(bool enable);
    bool isAllowLanTraffic() const;
    void setIsAllowLanTraffic(bool isAllowLanTraffic);

    types::ConnectionSettings connectionSettingsForNetworkInterface(const QString &networkOrSsid) const;

    const types::FirewallSettings &firewallSettings() const;
    void setFirewallSettings(const types::FirewallSettings &fs);
    const types::ConnectionSettings &connectionSettings() const;
    void setConnectionSettings(const types::ConnectionSettings &cs);
    const types::ApiResolutionSettings &apiResolutionSettings() const;
    void setApiResolutionSettings(const types::ApiResolutionSettings &drs);
    const types::ProxySettings &proxySettings() const;
    void setProxySettings(const types::ProxySettings &ps);
    DNS_POLICY_TYPE dnsPolicy() const;
    void setDnsPolicy(DNS_POLICY_TYPE policy);
    DNS_MANAGER_TYPE dnsManager() const;
    void setDnsManager(DNS_MANAGER_TYPE dnsManager);
    const types::MacAddrSpoofing &macAddrSpoofing() const;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing);
    const types::PacketSize &packetSize() const;
    void setPacketSize(const types::PacketSize &packetSize);
    UPDATE_CHANNEL updateChannel() const;
    void setUpdateChannel(UPDATE_CHANNEL channel);
    const types::ConnectedDnsInfo &connectedDnsInfo() const;
    void setConnectedDnsInfo(const types::ConnectedDnsInfo &info);
    const QMap<QString, types::ConnectionSettings> &networkPreferredProtocols() const;
    void setNetworkPreferredProtocols(const QMap<QString, types::ConnectionSettings> &settings);
    const types::Protocol networkLastKnownGoodProtocol(const QString &network) const;
    uint networkLastKnownGoodPort(const QString &network) const;
    void setNetworkLastKnownGoodProtocolPort(const QString &network, const types::Protocol &protocol, uint port);
    void clearLastKnownGoodProtocols(const QString &network);

    QString customOvpnConfigsPath() const;
    void setCustomOvpnConfigsPath(const QString &path);
    bool isKeepAliveEnabled() const;
    void setIsKeepAliveEnabled(bool enabled);

    bool operator==(const EngineSettings &other) const;
    bool operator!=(const EngineSettings &other) const;
    QJsonObject toJson() const;

    friend QDebug operator<<(QDebug dbg, const EngineSettings &es);

    friend LegacyProtobufSupport;

private:
    QSharedDataPointer<EngineSettingsData> d;

    // for serialization
    static constexpr quint32 magic_ = 0x7745C2AE;
    static constexpr int versionForSerialization_ = 5;  // should increment the version if the data format is changed
};

} // types namespace
