#include "enginesettings.h"

#include "legacy_protobuf_support/legacy_protobuf.h"
#include "types/global_consts.h"
#include "utils/extraconfig.h"
#include "utils/languagesutil.h"
#include "utils/logger.h"
#include "utils/simplecrypt.h"
#include "utils/utils.h"

const int typeIdEngineSettings = qRegisterMetaType<types::EngineSettings>("types::EngineSettings");

namespace types {

EngineSettings::EngineSettings() : d(new EngineSettingsData)
{
}

EngineSettings::EngineSettings(const QJsonObject &json) : d(new EngineSettingsData)
{
    const auto& jsonInfo = d->jsonInfo;

    if (json.contains(jsonInfo.kApiResolutionSettingsProp) && json[jsonInfo.kApiResolutionSettingsProp].isObject())
        d->apiResolutionSettings = types::ApiResolutionSettings(json[jsonInfo.kApiResolutionSettingsProp].toObject());

    if (json.contains(jsonInfo.kConnectedDnsInfoProp) && json[jsonInfo.kConnectedDnsInfoProp].isObject())
        d->connectedDnsInfo = types::ConnectedDnsInfo(json[jsonInfo.kConnectedDnsInfoProp].toObject());

    if (json.contains(jsonInfo.kConnectionSettingsProp) && json[jsonInfo.kConnectionSettingsProp].isObject())
        d->connectionSettings = types::ConnectionSettings(json[jsonInfo.kConnectionSettingsProp].toObject());

    if (json.contains(jsonInfo.kCustomOvpnConfigsPathProp) && json[jsonInfo.kCustomOvpnConfigsPathProp].isString())
        d->customOvpnConfigsPath = Utils::fromBase64(json[jsonInfo.kCustomOvpnConfigsPathProp].toString());

#if defined(Q_OS_LINUX)
    if (json.contains(jsonInfo.kDnsManagerProp) && json[jsonInfo.kDnsManagerProp].isDouble())
        d->dnsManager = static_cast<DNS_MANAGER_TYPE>(json[jsonInfo.kDnsManagerProp].toInt());
#endif

    if (json.contains(jsonInfo.kDnsPolicyProp) && json[jsonInfo.kDnsPolicyProp].isDouble())
        d->dnsPolicy = static_cast<DNS_POLICY_TYPE>(json[jsonInfo.kDnsPolicyProp].toInt());

    if (json.contains(jsonInfo.kFirewallSettingsProp) && json[jsonInfo.kFirewallSettingsProp].isObject())
        d->firewallSettings = types::FirewallSettings(json[jsonInfo.kFirewallSettingsProp].toObject());

    if (json.contains(jsonInfo.kIsAllowLanTrafficProp) && json[jsonInfo.kIsAllowLanTrafficProp].isBool())
        d->isAllowLanTraffic = json[jsonInfo.kIsAllowLanTrafficProp].toBool();

    if (json.contains(jsonInfo.kIsAntiCensorshipProp) && json[jsonInfo.kIsAntiCensorshipProp].isBool())
        d->isAntiCensorship = json[jsonInfo.kIsAntiCensorshipProp].toBool();

    if (json.contains(jsonInfo.kIsKeepAliveEnabledProp) && json[jsonInfo.kIsKeepAliveEnabledProp].isBool())
        d->isKeepAliveEnabled = json[jsonInfo.kIsKeepAliveEnabledProp].toBool();

#if defined(Q_OS_WIN)
    if (json.contains(jsonInfo.kIsTerminateSocketsProp) && json[jsonInfo.kIsTerminateSocketsProp].isBool())
        d->isTerminateSockets = json[jsonInfo.kIsTerminateSocketsProp].toBool();
#endif

    if (json.contains(jsonInfo.kLanguageProp) && json[jsonInfo.kLanguageProp].isString())
        d->language = json[jsonInfo.kLanguageProp].toString();

    if (json.contains(jsonInfo.kMacAddrSpoofingProp) && json[jsonInfo.kMacAddrSpoofingProp].isObject())
        d->macAddrSpoofing = types::MacAddrSpoofing(json[jsonInfo.kMacAddrSpoofingProp].toObject());

    if (json.contains(jsonInfo.kPacketSizeProp) && json[jsonInfo.kPacketSizeProp].isObject())
        d->packetSize = types::PacketSize(json[jsonInfo.kPacketSizeProp].toObject());

    if (json.contains(jsonInfo.kProxySettingsProp) && json[jsonInfo.kProxySettingsProp].isObject())
        d->proxySettings = types::ProxySettings(json[jsonInfo.kProxySettingsProp].toObject());

    if (json.contains(jsonInfo.kUpdateChannelProp) && json[jsonInfo.kUpdateChannelProp].isDouble())
        d->updateChannel = static_cast<UPDATE_CHANNEL>(json[jsonInfo.kUpdateChannelProp].toInt());

    if (json.contains(jsonInfo.kNetworkPreferredProtocolsProp) && json[jsonInfo.kNetworkPreferredProtocolsProp].isObject())
    {
        QMap<QString, types::ConnectionSettings> networkPreferredProtocols;
        const QJsonObject protocolsObj = json[jsonInfo.kNetworkPreferredProtocolsProp].toObject();
        for (const QString& networkBase64 : protocolsObj.keys())
        {
            if (protocolsObj[networkBase64].isObject())
                networkPreferredProtocols.insert(Utils::fromBase64(networkBase64), types::ConnectionSettings(protocolsObj[networkBase64].toObject()));
        }
        d->networkPreferredProtocols = networkPreferredProtocols;
    }

    if (json.contains(jsonInfo.kNetworkLastKnownGoodProtocolsProp) && json[jsonInfo.kNetworkLastKnownGoodProtocolsProp].isObject())
    {
        const QJsonObject protocolsObj = json[jsonInfo.kNetworkLastKnownGoodProtocolsProp].toObject();
        for (const QString& networkBase64 : protocolsObj.keys())
        {
            const QString network = Utils::fromBase64(networkBase64);
            if (protocolsObj[network].isObject())
            {
                types::Protocol protocol;
                uint port;
                const QJsonObject protocolObj = protocolsObj[network].toObject();
                if (protocolObj.contains(jsonInfo.kProtocolProp)
                    && protocolObj.contains(jsonInfo.kValueProp)
                    && protocolObj[jsonInfo.kProtocolProp].isDouble()
                    && protocolObj[jsonInfo.kValueProp].isDouble())
                {
                    protocol = static_cast<types::Protocol>(protocolObj[jsonInfo.kProtocolProp].toInt());
                    port = static_cast<uint>(protocolObj[jsonInfo.kValueProp].toInt());
                    setNetworkLastKnownGoodProtocolPort(network, protocol, port);
                }
            }
        }
    }
}

void EngineSettings::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << d->language << d->updateChannel << d->isIgnoreSslErrors << d->isTerminateSockets << d->isAllowLanTraffic <<
              d->firewallSettings << d->connectionSettings << d->apiResolutionSettings << d->proxySettings << d->packetSize <<
              d->macAddrSpoofing << d->dnsPolicy << d->tapAdapter << d->customOvpnConfigsPath << d->isKeepAliveEnabled <<
              d->connectedDnsInfo << d->dnsManager << d->networkPreferredProtocols << d->networkLastKnownGoodProtocols <<
              d->isAntiCensorship;
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("engineSettings", simpleCrypt.encryptToString(arr));
    settings.sync();
}

bool EngineSettings::loadFromSettings()
{
    bool bLoaded = false;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    QSettings settings;
    if (settings.contains("engineSettings")) {
        QString str = settings.value("engineSettings", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_) {
            ds >> version;
            ds >> d->language >> d->updateChannel >> d->isIgnoreSslErrors >> d->isTerminateSockets >> d->isAllowLanTraffic >>
                    d->firewallSettings >> d->connectionSettings >> d->apiResolutionSettings >> d->proxySettings >> d->packetSize >>
                    d->macAddrSpoofing >> d->dnsPolicy >> d->tapAdapter >> d->customOvpnConfigsPath >> d->isKeepAliveEnabled >>
                    d->connectedDnsInfo >> d->dnsManager;
            if (version >= 2) {
                ds >> d->networkPreferredProtocols;
            }
            if (version >= 3) {
                ds >> d->networkLastKnownGoodProtocols;
            }
            if (version >= 4) {
                ds >> d->isAntiCensorship;
            }
            if (version >= 5) {
                d->tapAdapter = WINTUN_ADAPTER;
            }
            if (ds.status() == QDataStream::Ok) {
                bLoaded = true;
            }
        }
    }

    if (!bLoaded && settings.contains("engineSettings2")) {
        // try load from legacy protobuf
        // todo remove this code at some point later
        QString str = settings.value("engineSettings2", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);
        if (LegacyProtobufSupport::loadEngineSettings(arr, *this)) {
            bLoaded = true;
        }

        settings.remove("engineSettings2");
    }

    if (!bLoaded) {
        qCDebug(LOG_BASIC) << "Could not load engine settings -- resetting to defaults";
        *this = EngineSettings();

        // Automatically enable anti-censorship feature for first-run users.
        if (LanguagesUtil::isCensorshipCountry()) {
            qCDebug(LOG_BASIC) << "Automatically enabled anti-censorship feature due to locale";
            // TODO: **JDRM** refactor this logic at some point so we don't have two sources of truth for the anti-censorship state.
            setIsAntiCensorship(true);
            ExtraConfig::instance().setAntiCensorship(true);
        }
    }

    if (d->language.isEmpty()) {
        d->language = LanguagesUtil::systemLanguage();
    }

    return bLoaded;
}

QString EngineSettings::language() const
{
    return d->language;
}

void EngineSettings::setLanguage(const QString &lang)
{
    d->language = lang;
}

bool EngineSettings::isIgnoreSslErrors() const
{
    return d->isIgnoreSslErrors;
}

void EngineSettings::setIsIgnoreSslErrors(bool ignore)
{
    d->isIgnoreSslErrors = ignore;
}

bool EngineSettings::isTerminateSockets() const
{
    return d->isTerminateSockets;
}

void EngineSettings::setIsTerminateSockets(bool close)
{
    d->isTerminateSockets = close;
}

bool EngineSettings::isAntiCensorship() const
{
    return d->isAntiCensorship;
}

void EngineSettings::setIsAntiCensorship(bool enable)
{
    d->isAntiCensorship = enable;
}

bool EngineSettings::isAllowLanTraffic() const
{
    return d->isAllowLanTraffic;
}

void EngineSettings::setIsAllowLanTraffic(bool isAllowLanTraffic)
{
    d->isAllowLanTraffic = isAllowLanTraffic;
}

ConnectionSettings EngineSettings::connectionSettingsForNetworkInterface(const QString &networkOrSsid) const
{
    if (!networkOrSsid.isEmpty() &&
        d->networkPreferredProtocols.contains(networkOrSsid) &&
        !d->networkPreferredProtocols[networkOrSsid].isAutomatic())
    {
        return d->networkPreferredProtocols[networkOrSsid];
    } else {
        return d->connectionSettings;
    }
}

const types::FirewallSettings &EngineSettings::firewallSettings() const
{
    return d->firewallSettings;
}

void EngineSettings::setFirewallSettings(const FirewallSettings &fs)
{
    d->firewallSettings = fs;
}

const types::ConnectionSettings &EngineSettings::connectionSettings() const
{
    return d->connectionSettings;
}

void EngineSettings::setConnectionSettings(const ConnectionSettings &cs)
{
    d->connectionSettings = cs;
}

const types::ApiResolutionSettings &EngineSettings::apiResolutionSettings() const
{
    return d->apiResolutionSettings;
}

void EngineSettings::setApiResolutionSettings(const ApiResolutionSettings &drs)
{
    d->apiResolutionSettings = drs;
}

const types::ProxySettings &EngineSettings::proxySettings() const
{
    return d->proxySettings;
}

void EngineSettings::setProxySettings(const ProxySettings &ps)
{
    d->proxySettings = ps;
}

DNS_POLICY_TYPE EngineSettings::dnsPolicy() const
{
    return d->dnsPolicy;
}

void EngineSettings::setDnsPolicy(DNS_POLICY_TYPE policy)
{
    d->dnsPolicy = policy;
}

DNS_MANAGER_TYPE EngineSettings::dnsManager() const
{
    return d->dnsManager;
}

void EngineSettings::setDnsManager(DNS_MANAGER_TYPE dnsManager)
{
    d->dnsManager = dnsManager;
}

const types::MacAddrSpoofing &EngineSettings::macAddrSpoofing() const
{
    return d->macAddrSpoofing;
}

const types::PacketSize &EngineSettings::packetSize() const
{
    return d->packetSize;
}

UPDATE_CHANNEL EngineSettings::updateChannel() const
{
    return d->updateChannel;
}

void EngineSettings::setUpdateChannel(UPDATE_CHANNEL channel)
{
    d->updateChannel = channel;
}

const types::ConnectedDnsInfo &EngineSettings::connectedDnsInfo() const
{
    return d->connectedDnsInfo;
}

void EngineSettings::setConnectedDnsInfo(const ConnectedDnsInfo &info)
{
    d->connectedDnsInfo = info;
}

QString EngineSettings::customOvpnConfigsPath() const
{
    return d->customOvpnConfigsPath;
}

void EngineSettings::setCustomOvpnConfigsPath(const QString &path)
{
    d->customOvpnConfigsPath = path;
}

bool EngineSettings::isKeepAliveEnabled() const
{
    return d->isKeepAliveEnabled;
}

void EngineSettings::setIsKeepAliveEnabled(bool enabled)
{
    d->isKeepAliveEnabled = enabled;
}

void EngineSettings::setMacAddrSpoofing(const MacAddrSpoofing &macAddrSpoofing)
{
    d->macAddrSpoofing = macAddrSpoofing;
}

void EngineSettings::setPacketSize(const PacketSize &packetSize)
{
    d->packetSize = packetSize;
}

const QMap<QString, types::ConnectionSettings> &EngineSettings::networkPreferredProtocols() const
{
    return d->networkPreferredProtocols;
}

void EngineSettings::setNetworkPreferredProtocols(const QMap<QString, types::ConnectionSettings> &preferredProtocols)
{
    d->networkPreferredProtocols = preferredProtocols;
}

const types::Protocol EngineSettings::networkLastKnownGoodProtocol(const QString &network) const
{
    return d->networkLastKnownGoodProtocols[network].first;
}

uint EngineSettings::networkLastKnownGoodPort(const QString &network) const
{
    return d->networkLastKnownGoodProtocols[network].second;
}

void EngineSettings::setNetworkLastKnownGoodProtocolPort(const QString &network, const types::Protocol &protocol, uint port)
{
    d->networkLastKnownGoodProtocols[network] = std::make_pair(protocol, port);
}

void EngineSettings::clearLastKnownGoodProtocols(const QString &network)
{
    if (network.isEmpty()) {
        d->networkLastKnownGoodProtocols.clear();
    } else {
        d->networkLastKnownGoodProtocols.remove(network);
    }
}

bool EngineSettings::operator==(const EngineSettings &other) const
{
    return  other.d->language == d->language &&
            other.d->updateChannel == d->updateChannel &&
            other.d->isIgnoreSslErrors == d->isIgnoreSslErrors &&
            other.d->isTerminateSockets == d->isTerminateSockets &&
            other.d->isAntiCensorship == d->isAntiCensorship &&
            other.d->isAllowLanTraffic == d->isAllowLanTraffic &&
            other.d->firewallSettings == d->firewallSettings &&
            other.d->connectionSettings == d->connectionSettings &&
            other.d->apiResolutionSettings == d->apiResolutionSettings &&
            other.d->proxySettings == d->proxySettings &&
            other.d->packetSize == d->packetSize &&
            other.d->macAddrSpoofing == d->macAddrSpoofing &&
            other.d->dnsPolicy == d->dnsPolicy &&
            other.d->tapAdapter == d->tapAdapter &&
            other.d->customOvpnConfigsPath == d->customOvpnConfigsPath &&
            other.d->isKeepAliveEnabled == d->isKeepAliveEnabled &&
            other.d->connectedDnsInfo == d->connectedDnsInfo &&
            other.d->dnsManager == d->dnsManager &&
            other.d->networkPreferredProtocols == d->networkPreferredProtocols &&
            other.d->networkLastKnownGoodProtocols == d->networkLastKnownGoodProtocols;
}

bool EngineSettings::operator!=(const EngineSettings &other) const
{
    return !(*this == other);
}

QJsonObject EngineSettings::toJson() const
{
    auto json = d->toJson();
    json[d->jsonInfo.kVersionProp] = static_cast<int>(versionForSerialization_);
    return json;
}

QDebug operator<<(QDebug dbg, const EngineSettings &es)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{language:" << es.d->language << "; ";
    dbg << "updateChannel:" << UPDATE_CHANNEL_toString(es.d->updateChannel) << "; ";
    dbg << "isIgnoreSslErrors:" << es.d->isIgnoreSslErrors << "; ";
    dbg << "isTerminateSockets:" << es.d->isTerminateSockets << "; ";
    dbg << "isAntiCensorship:" << es.d->isAntiCensorship << "; ";
    dbg << "isAllowLanTraffic:" << es.d->isAllowLanTraffic << "; ";
    dbg << "firewallSettings: " << es.d->firewallSettings << "; ";
    dbg << "connectionSettings: " << es.d->connectionSettings << "; ";
    dbg << "apiResolutionSettings: " << es.d->apiResolutionSettings << "; ";
    dbg << "proxySettings: " << es.d->proxySettings << "; ";
    dbg << "packetSize: " << es.d->packetSize << "; ";
    dbg << "macAddrSpoofing: " << es.d->macAddrSpoofing << "; ";
    dbg << "dnsPolicy: " << DNS_POLICY_TYPE_ToString(es.d->dnsPolicy) << "; ";
#ifdef Q_OS_WIN
    dbg << "tapAdapter: " << TAP_ADAPTER_TYPE_toString(es.d->tapAdapter) << "; ";
#endif
    dbg << "customOvpnConfigsPath: " << (es.d->customOvpnConfigsPath.isEmpty() ? "empty" : "settled") << "; ";
    dbg << "isKeepAliveEnabled:" << es.d->isKeepAliveEnabled << "; ";
    dbg << "connectedDnsInfo:" << es.d->connectedDnsInfo << "; ";
    dbg << "dnsManager:" << DNS_MANAGER_TYPE_toString(es.d->dnsManager) << "; ";
    dbg << "networkPreferredProtocols:" << es.d->networkPreferredProtocols << "; ";
    dbg << "networkLastKnownGoodProtocols: {" << es.d->networkPreferredProtocols << "; ";
    for (auto network : es.d->networkLastKnownGoodProtocols.keys()) {
        dbg << network << ": ";
        dbg << es.d->networkLastKnownGoodProtocols[network].first.toLongString() << ":";
        dbg << es.d->networkLastKnownGoodProtocols[network].second << "; ";
    }
    dbg << "}}";
    return dbg;
}

QJsonObject EngineSettingsData::toJson() const
{
    QJsonObject json;

    json[jsonInfo.kApiResolutionSettingsProp] = apiResolutionSettings.toJson();
    json[jsonInfo.kConnectedDnsInfoProp] = connectedDnsInfo.toJson();
    json[jsonInfo.kConnectionSettingsProp] = connectionSettings.toJson();
    json[jsonInfo.kCustomOvpnConfigsPathProp] = Utils::toBase64(customOvpnConfigsPath);
    json[jsonInfo.kDnsPolicyProp] = static_cast<int>(dnsPolicy);
    json[jsonInfo.kFirewallSettingsProp] = firewallSettings.toJson();
    json[jsonInfo.kIsAllowLanTrafficProp] = isAllowLanTraffic;
    json[jsonInfo.kIsAntiCensorshipProp] = isAntiCensorship;
    json[jsonInfo.kIsIgnoreSslErrorsProp] = isIgnoreSslErrors;
    json[jsonInfo.kIsKeepAliveEnabledProp] = isKeepAliveEnabled;
    json[jsonInfo.kIsTerminateSocketsProp] = isTerminateSockets;
    json[jsonInfo.kLanguageProp] = language;
    json[jsonInfo.kMacAddrSpoofingProp] = macAddrSpoofing.toJson();

    QJsonObject networkLastKnownGoodProtocolsObj;
    for (const auto& key : networkLastKnownGoodProtocols.keys()) {
        auto value = networkLastKnownGoodProtocols[key];
        QJsonObject innerObj;
        innerObj[jsonInfo.kProtocolProp] = value.first.toInt();
        innerObj[jsonInfo.kValueProp] = static_cast<int>(value.second);
        networkLastKnownGoodProtocolsObj[Utils::toBase64(key)] = innerObj;
    }
    json[jsonInfo.kNetworkLastKnownGoodProtocolsProp] = networkLastKnownGoodProtocolsObj;

    QJsonObject networkPreferredProtocolsObj;
    for (const auto& key : networkPreferredProtocols.keys()) {
        networkPreferredProtocolsObj[Utils::toBase64(key)] = networkPreferredProtocols[key].toJson();
    }
    json[jsonInfo.kNetworkPreferredProtocolsProp] = networkPreferredProtocolsObj;

    json[jsonInfo.kPacketSizeProp] = packetSize.toJson();
    json[jsonInfo.kProxySettingsProp] = proxySettings.toJson();
    json[jsonInfo.kTapAdapterProp] = static_cast<int>(tapAdapter);
    json[jsonInfo.kUpdateChannelProp] = static_cast<int>(updateChannel);
    return json;
}

} // types namespace
