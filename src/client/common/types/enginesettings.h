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
#include "types/decoytrafficsettings.h"

class LegacyProtobufSupport;

namespace types {

struct EngineSettingsData : public QSharedData
{
    EngineSettingsData() {}

    UPDATE_CHANNEL updateChannel = UPDATE_CHANNEL_RELEASE;
    bool isIgnoreSslErrors = false;
    bool isTerminateSockets = true;
    bool isAntiCensorship = false;
    bool isAllowLanTraffic = false;
    types::FirewallSettings firewallSettings;
    types::ConnectionSettings connectionSettings;
    types::ProxySettings proxySettings;
    types::PacketSize packetSize;
    types::MacAddrSpoofing macAddrSpoofing;
    DNS_POLICY_TYPE dnsPolicy = DNS_TYPE_CLOUDFLARE;
    TAP_ADAPTER_TYPE tapAdapter = WINTUN_ADAPTER;
    QString customOvpnConfigsPath;
    bool isKeepAliveEnabled = false;
    types::ConnectedDnsInfo connectedDnsInfo;
    DNS_MANAGER_TYPE dnsManager = DNS_MANAGER_AUTOMATIC;
    DecoyTrafficSettings decoyTrafficSettings;
    QMap<QString, types::ConnectionSettings> networkPreferredProtocols;
    QString language;

    void fromJson(const QJsonObject &json);
    QJsonObject toJson(bool isForDebugLog) const;
    void fromIni(QSettings &settings);
    void toIni(QSettings &settings) const;

private:
    static const inline QString kIniDnsManagerProp = "DNSManager";
    static const inline QString kIniDnsPolicyProp = "DNSPolicy";
    static const inline QString kIniIsAllowLanTrafficProp = "AllowLANTraffic";
    static const inline QString kIniIsAntiCensorshipProp = "CircumventCensorship";
    static const inline QString kIniIsIgnoreSslErrorsProp = "IgnoreSSLErrors";
    static const inline QString kIniIsKeepAliveEnabledProp = "ClientsideKeepalive";
    static const inline QString kIniLanguageProp = "Language";
    static const inline QString kIniUpdateChannelProp = "UpdateChannel";

    static const inline QString kJsonConnectedDnsInfoProp = "connectedDnsInfo";
    static const inline QString kJsonConnectionSettingsProp = "connectionSettings";
    static const inline QString kJsonCustomOvpnConfigsPathProp = "customOvpnConfigsPath";
    static const inline QString kJsonDnsManagerProp = "dnsManager";
    static const inline QString kJsonDnsPolicyProp = "dnsPolicy";
    static const inline QString kJsonDecoyTrafficSettingsProp = "decoyTrafficSettings";
    static const inline QString kJsonFirewallSettingsProp = "firewallSettings";
    static const inline QString kJsonIsAllowLanTrafficProp = "isAllowLanTraffic";
    static const inline QString kJsonIsAntiCensorshipProp = "isAntiCensorship";
    static const inline QString kJsonIsIgnoreSslErrorsProp = "isIgnoreSslErrors";
    static const inline QString kJsonIsKeepAliveEnabledProp = "isKeepAliveEnabled";
    static const inline QString kJsonIsTerminateSocketsProp = "isTerminateSockets";
    static const inline QString kJsonLanguageProp = "language";
    static const inline QString kJsonMacAddrSpoofingProp = "macAddrSpoofing";
    static const inline QString kJsonNetworkPreferredProtocolsProp = "networkPreferredProtocols";
    static const inline QString kJsonPacketSizeProp = "packetSize";
    static const inline QString kJsonProtocolProp = "protocol";
    static const inline QString kJsonProxySettingsProp = "proxySettings";
    static const inline QString kJsonTapAdapterProp = "tapAdapter";
    static const inline QString kJsonUpdateChannelProp = "updateChannel";
    static const inline QString kJsonValueProp = "value";
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

    QString customOvpnConfigsPath() const;
    void setCustomOvpnConfigsPath(const QString &path);
    bool isKeepAliveEnabled() const;
    void setIsKeepAliveEnabled(bool enabled);

    DecoyTrafficSettings decoyTrafficSettings() const;
    void setDecoyTrafficSettings(const DecoyTrafficSettings &decoyTrafficSettings);

    bool operator==(const EngineSettings &other) const;
    bool operator!=(const EngineSettings &other) const;
    QJsonObject toJson(bool isForDebugLog) const;
    void fromIni(QSettings &settings);
    void toIni(QSettings & settings) const;

    friend QDebug operator<<(QDebug dbg, const EngineSettings &es);

    friend LegacyProtobufSupport;

private:
    QSharedDataPointer<EngineSettingsData> d;

    static const inline QString kJsonVersionProp = "version";

    // for serialization
    static constexpr quint32 magic_ = 0x7745C2AE;
    static constexpr int versionForSerialization_ = 7;  // should increment the version if the data format is changed
};

} // types namespace
