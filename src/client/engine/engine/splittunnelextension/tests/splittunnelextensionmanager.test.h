#pragma once

#include <QObject>

class TestSplitTunnelExtensionManager : public QObject
{
    Q_OBJECT

private slots:
    void sessionFailureWaitsForReconnect();
    void startFailureUsesGenericReason();
    void recoveredOrReplacedSessionIsIgnored();
    void freshStateClassifiesSessionFailure_data();
    void freshStateClassifiesSessionFailure();
    void disableBeforeSessionFailure();
    void lateQueryReplyAfterStopIsIgnored_data();
    void lateQueryReplyAfterStopIsIgnored();
    void failedFreshQueryDoesNotReuseCachedActive_data();
    void failedFreshQueryDoesNotReuseCachedActive();
    void freshPropertiesOverrideCache_data();
    void freshPropertiesOverrideCache();
    void freshQueryDuringActivation();
    void freshReplyAcrossActivation_data();
    void freshReplyAcrossActivation();
};
