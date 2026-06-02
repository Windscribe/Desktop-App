#include "firewallcontroller_linux.h"
#include "utils/log/categories.h"

FirewallController_linux::FirewallController_linux(QObject *parent, Helper *helper) :
    FirewallController(parent), helper_(helper), forceUpdateVpnInterface_(false)
{
}

FirewallController_linux::~FirewallController_linux()
{
}

void FirewallController_linux::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, isVpnConnected);
    if (isStateChanged()) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count() + 1;
        // Commit the intent as the last-applied state only when the helper actually applied it, so a
        // failed apply leaves the baseline unchanged and the next firewallOn retries.
        if (firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig)) {
            commitFirewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, isVpnConnected);
            forceUpdateVpnInterface_ = false;
        }
    } else if (forceUpdateVpnInterface_) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall changed due to vpn interface update";
        // State is unchanged, so the baseline is already current; just clear the flag once the
        // interface refresh lands, leaving it set to retry on failure.
        if (firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig)) {
            forceUpdateVpnInterface_ = false;
        }
    }
}

void FirewallController_linux::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged()) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall off";
        // The helper tears down its own chains and the INPUT/OUTPUT jumps; the engine no longer
        // computes and sends a delete blob. Commit the off-state baseline only when the helper
        // actually cleared the rules, so a failed teardown leaves the baseline unchanged and the
        // next firewallOff retries instead of short-circuiting on an unchanged state.
        if (helper_->clearFirewallRules(false)) {
            commitFirewallOff();
        } else {
            qCWarning(LOG_FIREWALL_CONTROLLER) << "Clear firewall rules unsuccessful";
        }
    }
}

bool FirewallController_linux::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    return helper_->checkFirewallState();
}

bool FirewallController_linux::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    Q_UNUSED(ports);
    return true;
}

bool FirewallController_linux::deleteWhitelistPorts()
{
    return true;
}

void FirewallController_linux::setVpnInterface_posix(const QString &vpnInterface)
{
    QMutexLocker locker(&mutex_);
    qCInfo(LOG_BASIC) << "FirewallController_linux::setVpnInterface_posix ->" << vpnInterface;
    if (vpnInterface_ != vpnInterface) {
        vpnInterface_ = vpnInterface;
        forceUpdateVpnInterface_ = true;
    }
}

void FirewallController_linux::setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable, bool isAllowLanTraffic)
{
    Q_UNUSED(bEnable);
    helper_->setFirewallOnBoot(bEnable, ipTable, isAllowLanTraffic);
}

bool FirewallController_linux::firewallOnImpl(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    // Send only the validated intent. The helper builds the v4 and v6 iptables rules from a fixed
    // template and performs its own local-machine introspection (local addresses, hotspot adapter,
    // tunnel v6 capability) — none of that is trusted from this process.
    FirewallConfig config;
    config.connectingIp = connectingIp.toStdString();
    config.allowedIps.reserve(ips.size());
    for (const auto &ip : ips) {
        config.allowedIps.push_back(ip.toStdString());
    }
    config.allowLanTraffic = bAllowLanTraffic;
    config.isCustomConfig = bIsCustomConfig;
    config.vpnInterfaceName = vpnInterface_.toStdString();
    // staticPorts is macOS-only; Linux ignores it.

    bool ret = helper_->setFirewallRules(config);
    if (!ret) {
        qCCritical(LOG_FIREWALL_CONTROLLER) << "Could not set firewall rules";
    }

    return ret;
}
