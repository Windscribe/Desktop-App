#include "autoconnectionattemptstrategy.h"

#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

AutoConnectionAttemptStrategy::AutoConnectionAttemptStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const api_responses::PortMap &portMap,
                                               bool isProxyEnabled, bool isLockdownMode, bool skipWireguardProtocol, const QString &preferredNodeHostname)
    : locationInfo_(qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli)), portMap_(portMap),
      preferredNodeHostname_(preferredNodeHostname)
{
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());

    for (int portMapInd = 0; portMapInd < portMap_.const_items().count(); ++portMapInd) {
        const auto protocol = portMap_.const_items()[portMapInd].protocol;

        if (isProxyEnabled && protocol == types::Protocol::OPENVPN_UDP) {
            continue;
        }

        if (lockdownBlocksProtocol(isLockdownMode, protocol)) {
            continue;
        }

        if (skipWireguardProtocol && protocol.isWireGuardProtocol()) {
            continue;
        }

        // The API can hand us an item with no ports; every attempt below indexes ports[0], so a
        // malformed item must not enter the walk (WS_ASSERT is a no-op in release builds).
        if (portMap_.const_items()[portMapInd].ports.isEmpty()) {
            WS_ASSERT(false);
            continue;
        }

        AttemptInfo attemptInfo;
        attemptInfo.protocol = protocol;
        attemptInfo.portMapInd = portMapInd;

        // we attempt each protocol twice, so even indices are an initial attempt for a protocol and
        // odd numbers are a retry on a different node
        attemptInfo.changeNode = false;
        attempts_ << attemptInfo;
        attemptInfo.changeNode = true;
        attempts_ << attemptInfo;
    }

    selectNodeForAttempt(locationInfo_.data(), attempts_.value(0).protocol, preferredNodeHostname_, false);
}

void AutoConnectionAttemptStrategy::reset()
{
    curAttempt_ = 0;
    bIsAllFailed_ = false;
}

void AutoConnectionAttemptStrategy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << "Connection settings: automatic";
    qCInfo(LOG_CONNECTION) << locationInfo_->getLogString();
}

void AutoConnectionAttemptStrategy::putFailedConnection()
{
    if (curAttempt_ < (attempts_.count() - 1)) {
        curAttempt_++;
        if (attempts_[curAttempt_].changeNode) {
            selectNodeForAttempt(locationInfo_.data(), attempts_[curAttempt_].protocol, preferredNodeHostname_, true);
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

bool AutoConnectionAttemptStrategy::isFailed() const
{
    return bIsAllFailed_;
}

CurrentConnectionDescr AutoConnectionAttemptStrategy::getCurrentConnectionSettings() const
{
    // Every protocol can be filtered out (e.g. a WireGuard-only portmap under Always On+ with no
    // cached config); report an error node rather than indexing an empty walk.
    if (attempts_.isEmpty()) {
        return CurrentConnectionDescr();
    }
    return descrForSelectedNode(locationInfo_.data(), portMap_, attempts_[curAttempt_].protocol,
                                portMap_.const_items()[attempts_[curAttempt_].portMapInd].ports[0]);
}

bool AutoConnectionAttemptStrategy::isAutomaticMode()
{
    return true;
}


QVector<types::ProtocolStatus> AutoConnectionAttemptStrategy::protocolStatus()
{
    QVector<types::ProtocolStatus> status;
    QVector<types::ProtocolStatus> failedProtocols;
    QVector<types::ProtocolStatus> disconnectedProtocols;

    types::ProtocolStatus upNext;

    // A protocol is failed if it failed both attempts (i.e. an odd-indexed attempt failed), so just check the odd attempts
    for (int i = 1; i < attempts_.size(); i += 2) {
        types::ProtocolStatus::Status s;
        if (i - 1 == curAttempt_) {
            s = types::ProtocolStatus::Status::kUpNext;
            upNext = types::ProtocolStatus(attempts_[i].protocol, portMap_.const_items()[attempts_[i].portMapInd].ports[0], s, 10);
        } else if (i < curAttempt_ || bIsAllFailed_) {
            s = types::ProtocolStatus::Status::kFailed;
            failedProtocols.append(types::ProtocolStatus(attempts_[i].protocol, portMap_.const_items()[attempts_[i].portMapInd].ports[0], s, -1));
        } else {
            s = types::ProtocolStatus::Status::kDisconnected;
            disconnectedProtocols.append(types::ProtocolStatus(attempts_[i].protocol, portMap_.const_items()[attempts_[i].portMapInd].ports[0], s, -1));
        }
    }

    if (upNext.timeout > 0) {
        status.append(upNext);
    }
    status << disconnectedProtocols;
    status << failedProtocols;

    return status;
}

bool AutoConnectionAttemptStrategy::hasProtocolChanged()
{
    return (curAttempt_ % 2 == 0);
}
