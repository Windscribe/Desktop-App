#pragma once

#include "iconnectionattemptstrategy.h"
#include "engine/locationsmodel/mutablelocationinfo.h"
#include "api_responses/portmap.h"

// // manage automatic connection mode (only for API and static ips locations)
class AutoConnectionAttemptStrategy : public IConnectionAttemptStrategy
{
    Q_OBJECT
public:
    AutoConnectionAttemptStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const api_responses::PortMap &portMap,
                           bool isProxyEnabled, bool isLockdownMode, bool skipWireguardProtocol, const QString &preferredNodeHostname);

    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    bool hasProtocolChanged() override;
    types::Protocol preResolveProtocol() const override { return attempts_.value(curAttempt_).protocol; }

private:
    struct AttemptInfo
    {
        types::Protocol protocol;
        int portMapInd;
        bool changeNode;
    };

    QVector<AttemptInfo> attempts_;
    int curAttempt_ = 0;

    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    api_responses::PortMap portMap_;
    bool bIsAllFailed_ = false;
    QString preferredNodeHostname_;

    QVector<types::ProtocolStatus> protocolStatus();
};
