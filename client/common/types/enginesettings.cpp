#include "enginesettings.h"

#include <QJsonDocument>

#include "types/global_consts.h"
#include "utils/languagesutil.h"
#include "utils/log/categories.h"
#include "utils/simplecrypt.h"
#include "utils/utils.h"

const int typeIdEngineSettings = qRegisterMetaType<types::EngineSettings>("types::EngineSettings");

namespace types {

EngineSettings::EngineSettings() : d(new EngineSettingsData)
{
}

EngineSettings::EngineSettings(const QJsonObject &json) : d(new EngineSettingsData)
{
    d->fromJson(json);
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
            d->isAntiCensorship << d->decoyTrafficSettings;
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
            if (version >= 6) {
                ds >> d->decoyTrafficSettings;
            }
            if (ds.status() == QDataStream::Ok) {
                bLoaded = true;
            }
        }
    }

    if (!bLoaded) {
        qCDebug(LOG_BASIC) << "Could not load engine settings -- resetting to defaults";
        *this = EngineSettings();

        // Automatically enable anti-censorship feature for first-run users.
        if (LanguagesUtil::isCensorshipCountry()) {
            qCInfo(LOG_BASIC) << "Automatically enabled anti-censorship feature due to locale";
            // TODO: **JDRM** refactor this logic at some point so we don't have two sources of truth for the anti-censorship state.
            setIsAntiCensorship(true);
        }
    }

    if (d->language.isEmpty()) {
        d->language = LanguagesUtil::systemLanguage();
    }

#ifdef CLI_ONLY
    QSettings ini("Windscribe", "windscribe_cli");
    fromIni(ini);
#endif

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

DecoyTrafficSettings EngineSettings::decoyTrafficSettings() const
{
    return d->decoyTrafficSettings;
}

void EngineSettings::setDecoyTrafficSettings(const DecoyTrafficSettings &decoyTrafficSettings)
{
    d->decoyTrafficSettings = decoyTrafficSettings;
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
            other.d->decoyTrafficSettings == d->decoyTrafficSettings &&
            other.d->networkPreferredProtocols == d->networkPreferredProtocols &&
            other.d->networkLastKnownGoodProtocols == d->networkLastKnownGoodProtocols;
}

bool EngineSettings::operator!=(const EngineSettings &other) const
{
    return !(*this == other);
}

QJsonObject EngineSettings::toJson(bool isForDebugLog) const
{
    auto json = d->toJson(isForDebugLog);
    json[kJsonVersionProp] = static_cast<int>(versionForSerialization_);
    return json;
}

void EngineSettings::fromIni(QSettings &settings)
{
    d->fromIni(settings);
}

void EngineSettings::toIni(QSettings &settings) const
{
    d->toIni(settings);
}

QDebug operator<<(QDebug dbg, const EngineSettings &es)
{
    QJsonDocument doc(es.toJson(true));
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << doc.toJson(QJsonDocument::Compact);
    return dbg;
}

void EngineSettingsData::fromJson(const QJsonObject &json)
{
    if (json.contains(kJsonApiResolutionSettingsProp) && json[kJsonApiResolutionSettingsProp].isObject()) {
        apiResolutionSettings = types::ApiResolutionSettings(json[kJsonApiResolutionSettingsProp].toObject());
    }

    if (json.contains(kJsonConnectedDnsInfoProp) && json[kJsonConnectedDnsInfoProp].isObject()) {
        connectedDnsInfo = types::ConnectedDnsInfo(json[kJsonConnectedDnsInfoProp].toObject());
    }

    if (json.contains(kJsonConnectionSettingsProp) && json[kJsonConnectionSettingsProp].isObject()) {
        connectionSettings = types::ConnectionSettings(json[kJsonConnectionSettingsProp].toObject());
    }

    if (json.contains(kJsonCustomOvpnConfigsPathProp) && json[kJsonCustomOvpnConfigsPathProp].isString()) {
        customOvpnConfigsPath = Utils::fromBase64(json[kJsonCustomOvpnConfigsPathProp].toString());
    }

#if defined(Q_OS_LINUX)
    if (json.contains(kJsonDnsManagerProp) && json[kJsonDnsManagerProp].isDouble()) {
        dnsManager = DNS_MANAGER_TYPE_fromInt(json[kJsonDnsManagerProp].toInt());
    }
#endif

    if (json.contains(kJsonDnsPolicyProp) && json[kJsonDnsPolicyProp].isDouble()) {
        dnsPolicy = DNS_POLICY_TYPE_fromInt(json[kJsonDnsPolicyProp].toInt());
    }

    if (json.contains(kJsonDecoyTrafficSettingsProp) && json[kJsonDecoyTrafficSettingsProp].isObject()) {
        decoyTrafficSettings = types::DecoyTrafficSettings(json[kJsonDecoyTrafficSettingsProp].toObject());
    }

    if (json.contains(kJsonFirewallSettingsProp) && json[kJsonFirewallSettingsProp].isObject()) {
        firewallSettings = types::FirewallSettings(json[kJsonFirewallSettingsProp].toObject());
    }

    if (json.contains(kJsonIsAllowLanTrafficProp) && json[kJsonIsAllowLanTrafficProp].isBool()) {
        isAllowLanTraffic = json[kJsonIsAllowLanTrafficProp].toBool();
    }

    if (json.contains(kJsonIsAntiCensorshipProp) && json[kJsonIsAntiCensorshipProp].isBool()) {
        isAntiCensorship = json[kJsonIsAntiCensorshipProp].toBool();
    }

    if (json.contains(kJsonIsKeepAliveEnabledProp) && json[kJsonIsKeepAliveEnabledProp].isBool()) {
        isKeepAliveEnabled = json[kJsonIsKeepAliveEnabledProp].toBool();
    }

#if defined(Q_OS_WIN)
    if (json.contains(kJsonIsTerminateSocketsProp) && json[kJsonIsTerminateSocketsProp].isBool()) {
        isTerminateSockets = json[kJsonIsTerminateSocketsProp].toBool();
    }
#endif

    if (json.contains(kJsonLanguageProp) && json[kJsonLanguageProp].isString()) {
        QString lang = json[kJsonLanguageProp].toString();
        if (LanguagesUtil::isSupportedLanguage(lang)) {
            language = lang;
        }
    }

    if (json.contains(kJsonMacAddrSpoofingProp) && json[kJsonMacAddrSpoofingProp].isObject()) {
        macAddrSpoofing = types::MacAddrSpoofing(json[kJsonMacAddrSpoofingProp].toObject());
    }

    if (json.contains(kJsonPacketSizeProp) && json[kJsonPacketSizeProp].isObject()) {
        packetSize = types::PacketSize(json[kJsonPacketSizeProp].toObject());
    }

    if (json.contains(kJsonProxySettingsProp) && json[kJsonProxySettingsProp].isObject()) {
        proxySettings = types::ProxySettings(json[kJsonProxySettingsProp].toObject());
    }

    if (json.contains(kJsonUpdateChannelProp) && json[kJsonUpdateChannelProp].isDouble()) {
        updateChannel = UPDATE_CHANNEL_fromInt(json[kJsonUpdateChannelProp].toInt());
    }

    if (json.contains(kJsonNetworkPreferredProtocolsProp) && json[kJsonNetworkPreferredProtocolsProp].isObject()) {
        QMap<QString, types::ConnectionSettings> npp;
        const QJsonObject protocolsObj = json[kJsonNetworkPreferredProtocolsProp].toObject();
        for (const QString& networkBase64 : protocolsObj.keys()) {
            if (protocolsObj[networkBase64].isObject()) {
                npp.insert(Utils::fromBase64(networkBase64), types::ConnectionSettings(protocolsObj[networkBase64].toObject()));
            }
        }
        networkPreferredProtocols = npp;
    }

    if (json.contains(kJsonNetworkLastKnownGoodProtocolsProp) && json[kJsonNetworkLastKnownGoodProtocolsProp].isObject()) {
        const QJsonObject protocolsObj = json[kJsonNetworkLastKnownGoodProtocolsProp].toObject();
        for (const QString& networkBase64 : protocolsObj.keys()) {
            const QString network = Utils::fromBase64(networkBase64);
            if (protocolsObj[network].isObject()) {
                types::Protocol protocol;
                uint port;
                const QJsonObject protocolObj = protocolsObj[network].toObject();
                if (protocolObj.contains(kJsonProtocolProp)
                    && protocolObj.contains(kJsonValueProp)
                    && protocolObj[kJsonProtocolProp].isDouble()
                    && protocolObj[kJsonValueProp].isDouble())
                {
                    protocol = Protocol(protocolObj[kJsonProtocolProp].toInt());
                    port = protocolObj[kJsonValueProp].toInt();
                    if (port < 0 || port >= 65536) {
                        port = Protocol::defaultPortForProtocol(protocol);
                    }
                    networkLastKnownGoodProtocols[network] = std::make_pair(protocol, port);
                }
            }
        }
    }
}

QJsonObject EngineSettingsData::toJson(bool isForDebugLog) const
{
    QJsonObject json;

    json[kJsonApiResolutionSettingsProp] = apiResolutionSettings.toJson();
    json[kJsonConnectedDnsInfoProp] = connectedDnsInfo.toJson();
    json[kJsonConnectionSettingsProp] = connectionSettings.toJson(isForDebugLog);
    json[kJsonDnsPolicyProp] = static_cast<int>(dnsPolicy);
    json[kJsonDecoyTrafficSettingsProp] = decoyTrafficSettings.toJson();
    json[kJsonFirewallSettingsProp] = firewallSettings.toJson(isForDebugLog);
    json[kJsonIsAllowLanTrafficProp] = isAllowLanTraffic;
    json[kJsonIsAntiCensorshipProp] = isAntiCensorship;
    json[kJsonIsIgnoreSslErrorsProp] = isIgnoreSslErrors;
    json[kJsonIsKeepAliveEnabledProp] = isKeepAliveEnabled;
    json[kJsonIsTerminateSocketsProp] = isTerminateSockets;
    json[kJsonLanguageProp] = language;
    json[kJsonMacAddrSpoofingProp] = macAddrSpoofing.toJson(isForDebugLog);

    QJsonObject networkLastKnownGoodProtocolsObj;
    for (const auto& key : networkLastKnownGoodProtocols.keys()) {
        auto value = networkLastKnownGoodProtocols[key];
        QJsonObject innerObj;
        innerObj[kJsonProtocolProp] = value.first.toInt();
        innerObj[kJsonValueProp] = static_cast<int>(value.second);
        if (isForDebugLog) {
            innerObj["protocolDesc"] = value.first.toLongString();
            networkLastKnownGoodProtocolsObj[key] = innerObj;
        } else {
            networkLastKnownGoodProtocolsObj[Utils::toBase64(key)] = innerObj;
        }
    }
    json[kJsonNetworkLastKnownGoodProtocolsProp] = networkLastKnownGoodProtocolsObj;

    QJsonObject networkPreferredProtocolsObj;
    for (const auto& key : networkPreferredProtocols.keys()) {
        const auto jsonKey = isForDebugLog ? key : Utils::toBase64(key);
        networkPreferredProtocolsObj[jsonKey] = networkPreferredProtocols[key].toJson(isForDebugLog);
    }
    json[kJsonNetworkPreferredProtocolsProp] = networkPreferredProtocolsObj;

    json[kJsonPacketSizeProp] = packetSize.toJson();
    json[kJsonProxySettingsProp] = proxySettings.toJson(isForDebugLog);
    json[kJsonTapAdapterProp] = static_cast<int>(tapAdapter);
    json[kJsonUpdateChannelProp] = static_cast<int>(updateChannel);
    if (isForDebugLog) {
        // For log readability by humans/AI.
        json["dnsManagerDesc"] = DNS_MANAGER_TYPE_toString(dnsManager);
        json["dnsPolicyDesc"] = DNS_POLICY_TYPE_toString(dnsPolicy);
        json["updateChannelDesc"] = UPDATE_CHANNEL_toString(updateChannel);
    } else {
        json[kJsonCustomOvpnConfigsPathProp] = Utils::toBase64(customOvpnConfigsPath);
    }
    return json;
}

void EngineSettingsData::fromIni(QSettings &settings)
{
    QString lang = settings.value(kIniLanguageProp, language).toString();
    if (LanguagesUtil::isSupportedLanguage(lang)) {
        language = lang;
    }

    updateChannel = UPDATE_CHANNEL_fromString(settings.value(kIniUpdateChannelProp, UPDATE_CHANNEL_toString(updateChannel)).toString());

    settings.beginGroup(QString("Networks"));
    QMap<QString, types::ConnectionSettings> networkPreferredProtocols;
    for (const auto& key : settings.childKeys()) {
        ConnectionSettings connSettings(settings, key);
        networkPreferredProtocols.insert(key, connSettings);
    }
    settings.endGroup();

    settings.beginGroup(QString("Connection"));
    proxySettings.fromIni(settings);
    firewallSettings.fromIni(settings);
    connectionSettings.fromIni(settings);
    packetSize.fromIni(settings);
    macAddrSpoofing.fromIni(settings);
    connectedDnsInfo.fromIni(settings);
    isAllowLanTraffic = settings.value(kIniIsAllowLanTrafficProp, isAllowLanTraffic).toBool();
    decoyTrafficSettings.fromIni(settings);
    isAntiCensorship = settings.value(kIniIsAntiCensorshipProp, isAntiCensorship).toBool();
    settings.endGroup();

    settings.beginGroup(QString("Advanced"));
    apiResolutionSettings.fromIni(settings);
    isIgnoreSslErrors = settings.value(kIniIsIgnoreSslErrorsProp, isIgnoreSslErrors).toBool();
#ifdef Q_OS_LINUX
    dnsManager = DNS_MANAGER_TYPE_fromString(settings.value(kIniDnsManagerProp, DNS_MANAGER_TYPE_toString(dnsManager)).toString());
#endif
    dnsPolicy = DNS_POLICY_TYPE_fromString(settings.value(kIniDnsPolicyProp, DNS_POLICY_TYPE_toString(dnsPolicy)).toString());
    settings.endGroup();
}

void EngineSettingsData::toIni(QSettings &settings) const
{
    settings.setValue(kIniLanguageProp, language);
    settings.setValue(kIniUpdateChannelProp, UPDATE_CHANNEL_toString(updateChannel));

    settings.beginGroup(QString("Networks"));
    for (const auto& key : networkPreferredProtocols.keys()) {
        networkPreferredProtocols[key].toIni(settings, key);
    }
    settings.endGroup();

    settings.beginGroup(QString("Connection"));
    proxySettings.toIni(settings);
    firewallSettings.toIni(settings);
    connectionSettings.toIni(settings);
    packetSize.toIni(settings);
    macAddrSpoofing.toIni(settings);
    connectedDnsInfo.toIni(settings);
    settings.setValue(kIniIsAllowLanTrafficProp, isAllowLanTraffic);
    decoyTrafficSettings.toIni(settings);
    settings.setValue(kIniIsAntiCensorshipProp, isAntiCensorship);
    settings.endGroup();

    settings.beginGroup(QString("Advanced"));
    apiResolutionSettings.toIni(settings);
    settings.setValue(kIniIsIgnoreSslErrorsProp, isIgnoreSslErrors);
    settings.setValue(kIniDnsPolicyProp, DNS_POLICY_TYPE_toString(dnsPolicy));
#ifdef Q_OS_LINUX
    settings.setValue(kIniDnsManagerProp, DNS_MANAGER_TYPE_toString(dnsManager));
#endif
    settings.endGroup();
}

} // types namespace
