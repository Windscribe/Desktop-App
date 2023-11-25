#pragma once

#include "baseconnsettingspolicy.h"
#include "engine/locationsmodel/mutablelocationinfo.h"

// // manage automatic connection mode (only for API and static ips locations)
class AutoConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    AutoConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const types::PortMap &portMap, bool isProxyEnabled,
                           const types::Protocol protocol);

    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    void resolveHostnames() override;
    bool hasProtocolChanged() override;

private:
    struct AttemptInfo
    {
        types::Protocol protocol;
        int portMapInd;
        bool changeNode;
    };

    QVector<AttemptInfo> attempts_;
    int curAttempt_;

    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    types::PortMap portMap_;
    bool bIsAllFailed_;

    static types::Protocol lastKnownGoodProtocol_;
    static uint lastKnownGoodPort_;

    QVector<types::ProtocolStatus> protocolStatus();
};
