#include "firewallcontroller_mac.h"

#include <QSettings>

#include "utils/log/categories.h"
#include "utils/ws_assert.h"

FirewallController_mac::FirewallController_mac(QObject *parent, Helper *helper) :
    FirewallController(parent), helper_(helper), isAppFirewallEnabled_(false), isAllowLanTraffic_(false), isCustomConfig_(false), connectingIp_("")
{
    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (firewallState.isEnabled && !firewallState.isBasicAppRulesCorrect) {
        setPfWasEnabledState(true);
        isAppFirewallEnabled_ = false;
    } else if (firewallState.isEnabled && firewallState.isBasicAppRulesCorrect) {
        allowedIps_ = firewallState.allowedIps;
        if (allowedIps_.isEmpty()) {
            qCWarning(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but" << WS_PRODUCT_NAME_LOWER "_ips table not found or empty.";
            WS_ASSERT(false);
        }
        vpnInterface_ = firewallState.vpnInterface;
        isAllowLanTraffic_ = firewallState.isAllowLanTraffic;
        isAppFirewallEnabled_ = true;
    } else {
        setPfWasEnabledState(false);
        isAppFirewallEnabled_ = false;
    }
}

void FirewallController_mac::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    Q_UNUSED(isVpnConnected);
    QMutexLocker locker(&mutex_);
    FirewallState firewallState;

    checkInternalVsPfctlState(&firewallState);

    if (!isAppFirewallEnabled_) {
        setPfWasEnabledState(firewallState.isEnabled);
    }

    // Once the firewall is up, skip the push when none of our inputs changed: each push makes the
    // helper re-walk SystemConfiguration (getOsDnsServers) to diff <disallowed_dns>, so re-sending
    // an identical intent is wasted work. The interface and static-ports inputs arrive through their
    // own setters (setVpnInterface_posix / whitelistPorts), which push themselves, so they aren't
    // part of this comparison. The cached fields only ever hold the last successfully applied
    // intent, so a failed push leaves them unchanged and the next call retries rather than skipping.
    if (isAppFirewallEnabled_ &&
        connectingIp_ == connectingIp &&
        allowedIps_ == ips &&
        isAllowLanTraffic_ == bAllowLanTraffic &&
        isCustomConfig_ == bIsCustomConfig) {
        return;
    }

    // Build the intent from the new inputs (interface and static ports stay owned by their setters)
    // and commit it to the cached fields only once the helper has applied it. If the helper fails
    // (e.g. a pf parse/load error) it leaves the prior rules intact; not caching here keeps the
    // engine from reporting protection that isn't there and lets the next call re-attempt.
    FirewallConfig config = makeConfig();
    config.connectingIp = connectingIp.toStdString();
    config.allowedIps.clear();
    config.allowedIps.reserve(ips.size());
    for (const auto &ip : ips) {
        config.allowedIps.push_back(ip.toStdString());
    }
    config.allowLanTraffic = bAllowLanTraffic;
    config.isCustomConfig = bIsCustomConfig;

    if (applyConfig(config)) {
        connectingIp_ = connectingIp;
        allowedIps_ = ips;
        isAllowLanTraffic_ = bAllowLanTraffic;
        isCustomConfig_ = bIsCustomConfig;
    }
}

void FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();
    firewallOffImpl();
    isAppFirewallEnabled_ = false;
    allowedIps_.clear();
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();
    return isAppFirewallEnabled_;
}

bool FirewallController_mac::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    checkInternalVsPfctlState();

    if (staticIpPorts_ == ports) {
        return true;
    }

    // Not enabled yet: just remember the ports; the next firewallOn applies them as part of the
    // full push. When enabled, commit the cached ports only once the helper has applied them.
    if (!isAppFirewallEnabled_) {
        staticIpPorts_ = ports;
        return true;
    }

    FirewallConfig config = makeConfig();
    config.staticPorts.clear();
    config.staticPorts.reserve(ports.size());
    for (unsigned int port : ports) {
        config.staticPorts.push_back(port);
    }
    if (applyConfig(config)) {
        staticIpPorts_ = ports;
    }

    return true;
}

bool FirewallController_mac::deleteWhitelistPorts()
{
    return whitelistPorts(api_responses::StaticIpPortsVector());
}

void FirewallController_mac::firewallOffImpl()
{
    helper_->clearFirewallRules(isPfWasEnabled());
    isAppFirewallEnabled_ = false;
    qCInfo(LOG_FIREWALL_CONTROLLER) << "firewallOff disabled";
}

void FirewallController_mac::getFirewallStateFromPfctl(FirewallState &outState)
{
    outState.isEnabled = helper_->checkFirewallState();

    QString output;
    bool ret;

    // checking a few key rules to make sure the firewall rules were set by our program
    helper_->getFirewallRules(kFirewallAllRules, output);

    if (output.indexOf("anchor \"" WS_PRODUCT_NAME_LOWER "_vpn_traffic\" all") != -1 &&
        output.indexOf("anchor \"" WS_PRODUCT_NAME_LOWER "_lan_traffic\" all") != -1 &&
        output.indexOf("anchor \"" WS_PRODUCT_NAME_LOWER "_static_ports_traffic\" all") != -1)
    {
        outState.isBasicAppRulesCorrect = true;
    } else {
        outState.isBasicAppRulesCorrect = false;
    }

    // get allowed IPs table
    output.clear();
    outState.allowedIps.clear();
    ret = helper_->getFirewallRules(kFirewallIpsTable, output);
    if (ret && !output.isEmpty()) {
        const QStringList list = output.split("\n");
        for (auto &ip : list) {
            if (!ip.trimmed().isEmpty()) {
                outState.allowedIps << ip.trimmed();
            }
        }
    }
}

bool FirewallController_mac::checkInternalVsPfctlState(FirewallState *outFirewallState /*= nullptr*/)
{
    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (outFirewallState)
        *outFirewallState = firewallState;

    if (firewallState.isEnabled && firewallState.isBasicAppRulesCorrect) {
        if (!isAppFirewallEnabled_) {
            WS_ASSERT("Firewall state doesn't match");
            return false;
        }
        if (allowedIps_ != firewallState.allowedIps) {
            WS_ASSERT("Allowed IPs don't match");
            return false;
        }
    }
    return true;
}

FirewallConfig FirewallController_mac::makeConfig() const
{
    FirewallConfig config;
    config.connectingIp = connectingIp_.toStdString();
    config.allowedIps.reserve(allowedIps_.size());
    for (const auto &ip : allowedIps_) {
        config.allowedIps.push_back(ip.toStdString());
    }
    config.allowLanTraffic = isAllowLanTraffic_;
    config.isCustomConfig = isCustomConfig_;
    config.vpnInterfaceName = vpnInterface_.toStdString();
    config.staticPorts.reserve(staticIpPorts_.size());
    for (unsigned int port : staticIpPorts_) {
        config.staticPorts.push_back(port);
    }
    return config;
}

bool FirewallController_mac::applyConfig(const FirewallConfig &config)
{
    if (!helper_->setFirewallRules(config)) {
        qCCritical(LOG_FIREWALL_CONTROLLER) << "Failed to apply firewall rules";
        return false;
    }
    isAppFirewallEnabled_ = true;
    return true;
}

void FirewallController_mac::setVpnInterface_posix(const QString &vpnInterface)
{
    QMutexLocker locker(&mutex_);
    qCDebug(LOG_BASIC) << "FirewallController_mac::setVpnInterface_posix ->" << vpnInterface;
    checkInternalVsPfctlState();

    // Not enabled yet: just remember the interface; the next firewallOn applies it. When enabled,
    // commit the cached interface only once the helper has applied it, so a failed push retries.
    if (!isAppFirewallEnabled_) {
        vpnInterface_ = vpnInterface;
        return;
    }

    FirewallConfig config = makeConfig();
    config.vpnInterfaceName = vpnInterface.toStdString();
    if (applyConfig(config)) {
        vpnInterface_ = vpnInterface;
    }
}

void FirewallController_mac::setFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable, bool isAllowLanTraffic)
{
    helper_->setFirewallOnBoot(bEnable, ipTable, isAllowLanTraffic);
}

void FirewallController_mac::setPfWasEnabledState(bool b)
{
    QSettings settings;
    settings.setValue("pfIsEnabled", b);
}

bool FirewallController_mac::isPfWasEnabled() const
{
    QSettings settings;
    return settings.value("pfIsEnabled").toBool();
}
