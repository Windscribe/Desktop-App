#include "manualconnectionattemptstrategy.h"

#include "utils/log/categories.h"
#include "utils/ws_assert.h"

ManualConnectionAttemptStrategy::ManualConnectionAttemptStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli,
    const types::ConnectionSettings &connectionSettings, const api_responses::PortMap &portMap, const QString &preferredNodeHostname) :
        locationInfo_(qSharedPointerDynamicCast<locationsmodel::MutableLocationInfo>(bli)),
        portMap_(portMap), connectionSettings_(connectionSettings), failedManualModeCounter_(0), preferredNodeHostname_(preferredNodeHostname)
{
    WS_ASSERT(!locationInfo_.isNull());
    WS_ASSERT(!locationInfo_->locationId().isCustomConfigsLocation());

    selectNodeForAttempt(locationInfo_.data(), connectionSettings_.protocol(), preferredNodeHostname_, false);
}

void ManualConnectionAttemptStrategy::reset()
{
    failedManualModeCounter_ = 0;
}

void ManualConnectionAttemptStrategy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << "Connection settings:" << connectionSettings_.toJson(true);
    qCInfo(LOG_CONNECTION) << locationInfo_->getLogString();
}

void ManualConnectionAttemptStrategy::putFailedConnection()
{
    failedManualModeCounter_++;

    if (failedManualModeCounter_ < 2) {
        selectNodeForAttempt(locationInfo_.data(), connectionSettings_.protocol(), preferredNodeHostname_, true);
    } else {
        QVector<types::ProtocolStatus> status;
        emit protocolStatusChanged(status, false);
    }
}

bool ManualConnectionAttemptStrategy::isFailed() const
{
    return failedManualModeCounter_ >= 2;
}

CurrentConnectionDescr ManualConnectionAttemptStrategy::getCurrentConnectionSettings() const
{
    return descrForSelectedNode(locationInfo_.data(), portMap_, connectionSettings_.protocol(), connectionSettings_.port());
}

bool ManualConnectionAttemptStrategy::isAutomaticMode()
{
    return false;
}
