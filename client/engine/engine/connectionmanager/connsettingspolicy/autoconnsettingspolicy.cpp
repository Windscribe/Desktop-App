#include "autoconnsettingspolicy.h"

#include <QDataStream>
#include <QSettings>
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "utils/logger.h"
#include "utils/ws_assert.h"

types::Protocol AutoConnSettingsPolicy::lastKnownGoodProtocol_;

AutoConnSettingsPolicy::AutoConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                               const types::PortMap &portMap, bool isProxyEnabled,
                                               const types::Protocol protocol)
{
    attempts_.clear();
    curAttempt_ = 0;
    bIsAllFailed_ = false;
    portMap_ = portMap;
    locationInfo_ = qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli);
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());

    if (protocol.isValid()) {
        lastKnownGoodProtocol_ = protocol;
    }

    for (int portMapInd = 0; portMapInd < portMap_.items().count(); ++portMapInd) {
        // skip udp protocol, if proxy enabled
        if (isProxyEnabled && portMap_.items()[portMapInd].protocol == types::Protocol::OPENVPN_UDP) {
            continue;
        }

        AttemptInfo attemptInfo;
        attemptInfo.protocol = portMap_.items()[portMapInd].protocol;
        WS_ASSERT(portMap_.items()[portMapInd].ports.count() > 0);
        attemptInfo.portMapInd = portMapInd;

        // we attempt each protocol twice, so even indices are an initial attempt for a protocol and
        // odd numbers are a retry on a different node
        if (attemptInfo.protocol == lastKnownGoodProtocol_) {
            // prepend in reverse order
            attemptInfo.changeNode = true;
            attempts_.prepend(attemptInfo);
            attemptInfo.changeNode = false;
            attempts_.prepend(attemptInfo);
        } else {
            attemptInfo.changeNode = false;
            attempts_ << attemptInfo;
            attemptInfo.changeNode = true;
            attempts_ << attemptInfo;
        }
    }

    QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (IpValidation::isIp(remoteOverride) && attempts_.size() > 0 && attempts_[0].protocol == types::Protocol::WIREGUARD) {
        locationInfo_->selectNodeByIp(remoteOverride);
    }
}

void AutoConnSettingsPolicy::reset()
{
    curAttempt_ = 0;
    bIsAllFailed_ = false;
}

void AutoConnSettingsPolicy::debugLocationInfoToLog() const
{
    qCDebug(LOG_CONNECTION) << "Connection settings: automatic";
    qCDebug(LOG_CONNECTION) << locationInfo_->getLogString();
}

void AutoConnSettingsPolicy::putFailedConnection()
{
    if (!bStarted_) {
        return;
    }

    if (curAttempt_ < (attempts_.count() - 1)) {
        if (attempts_[curAttempt_].changeNode) {
            QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
            if (IpValidation::isIp(remoteOverride) && attempts_[curAttempt_ + 1].protocol == types::Protocol::WIREGUARD) {
                locationInfo_->selectNodeByIp(remoteOverride);
            } else {
                locationInfo_->selectNextNode();
            }
        }
        curAttempt_++;
        // even indicies are a new protocol, so emit a change
        if (curAttempt_ % 2 == 0) {
            emit protocolStatusChanged(protocolStatus());
        }
    } else {
        bIsAllFailed_ = true;
        emit protocolStatusChanged(protocolStatus());
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

    QString remoteOverride = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (IpValidation::isIp(remoteOverride) && ccd.protocol == types::Protocol::WIREGUARD) {
        ccd.ip = remoteOverride;
    } else {
        int useIpInd = portMap_.getUseIpInd(ccd.protocol);
        ccd.ip = locationInfo_->getIpForSelectedNode(useIpInd);
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

void AutoConnSettingsPolicy::resolveHostnames()
{
    // nothing todo
    emit hostnamesResolved();
}

QVector<types::ProtocolStatus> AutoConnSettingsPolicy::protocolStatus() {
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