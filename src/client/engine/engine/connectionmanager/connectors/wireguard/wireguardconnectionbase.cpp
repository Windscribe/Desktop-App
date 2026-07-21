#include "wireguardconnectionbase.h"

#include "engine/wireguardconfig/getwireguardconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

WireGuardConnectionBase::WireGuardConnectionBase(QObject *parent, types::Protocol protocol, const WireGuardSessionParams &sessionParams)
    : IConnection(parent, protocol), sessionParams_(sessionParams)
{
    capabilities_ = wireGuardConnectorCapabilities();
}

void WireGuardConnectionBase::prepare(const CurrentConnectionDescr &descr, const AttemptEnvironment &env)
{
    descr_ = descr;
    env_ = env;
    protocol_ = descr.protocol;

    if (descr.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG) {
        WS_ASSERT(descr.wgCustomConfig != nullptr);
        if (descr.wgCustomConfig == nullptr) {
            qCWarning(LOG_CONNECTION) << "Failed to get config for custom WG file:" << descr.customConfigFilename;
            emit prepareFailed(CONNECT_ERROR::CANNOT_OPEN_CUSTOM_CONFIG);
            return;
        }
        // Honour the "IP Stack" preference: if the user picked IPv4 Only, strip every v6 entry so
        // the tunnel never carries IPv6 traffic even if the .conf file is dual-stack.
        if (sessionParams_.ipStackEgress == IpStack::kIPv4Only) {
            config_ = descr.wgCustomConfig->stripIpv6Addresses();
        } else {
            config_ = *descr.wgCustomConfig;
        }
        emit prepared();
        return;
    }

    qCInfo(LOG_CONNECTION) << "Requesting WireGuard config for hostname =" << descr_.hostname << "isIpv6Support = " << descr_.isIpv6Support;
    fetchConfig(false);
}

WireGuardConfig WireGuardConnectionBase::dialConfig() const
{
    WireGuardConfig config = config_;
    // override the DNS if we are using custom
    if (!env_.primaryDnsServer.isEmpty()) {
        config.setClientDnsAddress(env_.primaryDnsServer);
    }
    return config;
}

void WireGuardConnectionBase::teardown()
{
    SAFE_DELETE(getWireGuardConfig_);
}

void WireGuardConnectionBase::continueWithUserInput(const UserInputResponse &response)
{
    // Arrival of the consent is the user's instruction to release the oldest key and refetch; any
    // other response was meant for another attempt's connector and is silently ignored.
    if (std::get_if<KeyLimitConsentResponse>(&response) != nullptr) {
        fetchConfig(true);
    }
}

void WireGuardConnectionBase::fetchConfig(bool deleteOldestKey)
{
    SAFE_DELETE(getWireGuardConfig_);
    getWireGuardConfig_ = new GetWireGuardConfig(this);
    connect(getWireGuardConfig_, &GetWireGuardConfig::getWireGuardConfigAnswer, this, &WireGuardConnectionBase::onGetWireGuardConfigAnswer);
    getWireGuardConfig_->getWireGuardConfig(descr_.hostname, deleteOldestKey, env_.configFetchMode == ConfigFetchMode::CachedOnly);
}

void WireGuardConnectionBase::onGetWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config)
{
    if (retCode == WireGuardConfigRetCode::kKeyLimit) {
        emit userInputRequired(UserInputType::KeyLimitConsent);
        return;
    }
    if (retCode == WireGuardConfigRetCode::kFailoverFailed) {
        // All options for accessing the API have been exhausted.
        emit prepareFailed(CONNECT_ERROR::WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
        return;
    }
    if (retCode != WireGuardConfigRetCode::kSuccess) {
        emit prepareFailed(CONNECT_ERROR::CONFIG_FETCH_FAILED);
        return;
    }

    // Dual-stack tunnel only when the selected node advertises IPv6 *and* the user has
    // not forced IPv4-only via the "IP Stack" preference. Auto leaves the v6 portion
    // intact (server-supplied client v6 address + ::/0 in AllowedIPs); IPv4 Only strips
    // every v6 entry from Address, DNS and AllowedIPs.
    const bool dualStack = descr_.isIpv6Support && sessionParams_.ipStackEgress == IpStack::kAuto;
    if (dualStack) {
        config_ = config;
    } else {
        config_ = config.stripIpv6Addresses();
    }

    if (!NetworkingValidation::isIp(descr_.ip)) {
        qCCritical(LOG_CONNECTION) << "Refusing to build WireGuard endpoint, node IP is not a valid IP address";
        emit prepareFailed(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
        return;
    }
    config_.setPeerPublicKey(descr_.wgPeerPublicKey);
    config_.setPeerEndpoint(QString("%1:%2").arg(descr_.ip).arg(descr_.port));

    if (!sessionParams_.amneziawgPreset.isEmpty()) {
        config_.setAmneziawgParam(sessionParams_.amneziawgParams.getUnblockParamForPreset(sessionParams_.amneziawgPreset));
    } else {
        config_.setAmneziawgParam(api_responses::AmneziawgUnblockParam());
    }

    emit prepared();
}
