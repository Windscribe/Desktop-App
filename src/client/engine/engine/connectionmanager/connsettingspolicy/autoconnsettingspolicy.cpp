#include "autoconnsettingspolicy.h"

#include <QDataStream>
#include <QSettings>
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_MACOS
#include "utils/macutils.h"
#endif

AutoConnSettingsPolicy::AutoConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const api_responses::PortMap &portMap,
                                               bool isProxyEnabled, bool skipWireguardProtocol, const QString &preferredNodeHostname)
{
    attempts_.clear();
    curAttempt_ = 0;
    bIsAllFailed_ = false;
    portMap_ = portMap;
    preferredNodeHostname_ = preferredNodeHostname;
    locationInfo_ = qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli);
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());

    for (int portMapInd = 0; portMapInd < portMap_.items().count(); ++portMapInd) {
        const auto protocol = portMap_.items()[portMapInd].protocol;

        if (isProxyEnabled && protocol == types::Protocol::OPENVPN_UDP) {
            continue;
        }

#ifdef Q_OS_MACOS
        if (MacUtils::isLockdownMode() && protocol.isIkev2Protocol()) {
            continue;
        }
#endif

        if (skipWireguardProtocol && protocol.isWireGuardProtocol()) {
            continue;
        }

        AttemptInfo attemptInfo;
        attemptInfo.protocol = protocol;
        WS_ASSERT(portMap_.items()[portMapInd].ports.count() > 0);
        attemptInfo.portMapInd = portMapInd;

        // we attempt each protocol twice, so even indices are an initial attempt for a protocol and
        // odd numbers are a retry on a different node
        attemptInfo.changeNode = false;
        attempts_ << attemptInfo;
        attemptInfo.changeNode = true;
        attempts_ << attemptInfo;
    }

    QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (IpValidation::isIp(remoteOverride) && attempts_.size() > 0 && attempts_[0].protocol == types::Protocol::WIREGUARD && !remoteOverride.isEmpty()) {
        locationInfo_->selectNodeByIp(remoteOverride);
    } else if (!preferredNodeHostname.isEmpty()) {
        qCInfo(LOG_CONNECTION) << "Selecting preferred node by hostname: " << preferredNodeHostname;
        if (locationInfo_->selectNodeByHostname(preferredNodeHostname)) {
            qCInfo(LOG_CONNECTION) << "Found matching node: " << locationInfo_->getLogString();
        }
    }
}

void AutoConnSettingsPolicy::reset()
{
    curAttempt_ = 0;
    bIsAllFailed_ = false;
}

void AutoConnSettingsPolicy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << "Connection settings: automatic";
    qCInfo(LOG_CONNECTION) << locationInfo_->getLogString();
}

void AutoConnSettingsPolicy::putFailedConnection()
{
    if (!bStarted_) {
        return;
    }

    if (curAttempt_ < (attempts_.count() - 1)) {
        curAttempt_++;
        if (attempts_[curAttempt_].changeNode) {
            QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
            if (IpValidation::isIp(remoteOverride) && attempts_[curAttempt_].protocol == types::Protocol::WIREGUARD) {
                locationInfo_->selectNodeByIp(remoteOverride);
            } else if (!preferredNodeHostname_.isEmpty()) {
                qCInfo(LOG_CONNECTION) << "Selecting preferred node by hostname on retry: " << preferredNodeHostname_;
                if (!locationInfo_->selectNodeByHostname(preferredNodeHostname_)) {
                    locationInfo_->selectNextNode();
                }
            } else {
                locationInfo_->selectNextNode();
            }
        }
        // even indicies are a new protocol, so emit a change
        if (curAttempt_ % 2 == 0) {
            emit protocolStatusChanged(protocolStatus(), true);
        }
    } else {
        bIsAllFailed_ = true;
        emit protocolStatusChanged(protocolStatus(), true);
    }
}

bool AutoConnSettingsPolicy::isFailed() const
{
    if (!bStarted_) {
        return false;
    }
    return bIsAllFailed_;
}

CurrentConnectionDescr AutoConnSettingsPolicy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;

    ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
    ccd.protocol = attempts_[curAttempt_].protocol;
    ccd.port = portMap_.const_items()[attempts_[curAttempt_].portMapInd].ports[0];

    int useIpInd = portMap_.getUseIpInd(ccd.protocol);
    ccd.ip = locationInfo_->getIpForSelectedNode(useIpInd);
    if (ccd.ip.isEmpty()) {
        qCWarning(LOG_CONNECTION) << "Could not get IP for selected node.  Port map: ";
        for (const auto &item : portMap_.const_items()) {
            qCWarning(LOG_CONNECTION) << "protocol:" << item.protocol.toLongString() << "heading:" << item.heading << "use:" << item.use << "ports:" << item.ports;
        }
    }
    ccd.hostname = locationInfo_->getHostnameForSelectedNode();
    ccd.dnsHostName = locationInfo_->getDnsName();
    ccd.wgPeerPublicKey = locationInfo_->getWgPubKeyForSelectedNode();
    ccd.verifyX509name = locationInfo_->getVerifyX509name();

    // for static IP set additional fields
    if (locationInfo_->locationId().isStaticIpsLocation()) {
        ccd.connectionNodeType = CONNECTION_NODE_STATIC_IPS;
        ccd.username = locationInfo_->getStaticIpUsername();
        ccd.password = locationInfo_->getStaticIpPassword();
        ccd.staticIpPorts = locationInfo_->getStaticIpPorts();

        // for static ip with wireguard protocol override id to wg_ip
        if (ccd.protocol == types::Protocol::WIREGUARD) {
            ccd.ip = locationInfo_->getWgIpForSelectedNode();
        }
    }

    return ccd;
}

bool AutoConnSettingsPolicy::isAutomaticMode()
{
    return true;
}

bool AutoConnSettingsPolicy::isCustomConfig()
{
    return false;
}

void AutoConnSettingsPolicy::resolveHostnames()
{
    // nothing todo
    emit hostnamesResolved();
}

QVector<types::ProtocolStatus> AutoConnSettingsPolicy::protocolStatus()
{
    QVector<types::ProtocolStatus> status;
    QVector<types::ProtocolStatus> failedProtocols;
    QVector<types::ProtocolStatus> disconnectedProtocols;
    types::Protocol lastProtocol(types::Protocol::TYPE::UNINITIALIZED);
    bool failed = true;

    types::ProtocolStatus upNext;

    // A protocol is failed if it failed both attempts (i.e. an odd-indexed attempt failed), so just check the odd attempts
    for (int i = 1; i < attempts_.size(); i += 2) {
        types::ProtocolStatus::Status s;
        if (i - 1 == curAttempt_) {
            s = types::ProtocolStatus::Status::kUpNext;
            upNext = types::ProtocolStatus(attempts_[i].protocol, portMap_.items()[attempts_[i].portMapInd].ports[0], s, 10);
        } else if (i < curAttempt_ || bIsAllFailed_) {
            s = types::ProtocolStatus::Status::kFailed;
            failedProtocols.append(types::ProtocolStatus(attempts_[i].protocol, portMap_.items()[attempts_[i].portMapInd].ports[0], s, -1));
        } else {
            s = types::ProtocolStatus::Status::kDisconnected;
            disconnectedProtocols.append(types::ProtocolStatus(attempts_[i].protocol, portMap_.items()[attempts_[i].portMapInd].ports[0], s, -1));
        }
    }

    if (upNext.timeout > 0) {
        status.append(upNext);
    }
    status << disconnectedProtocols;
    status << failedProtocols;

    return status;
}

bool AutoConnSettingsPolicy::hasProtocolChanged()
{
    return (curAttempt_ % 2 == 0);
}
