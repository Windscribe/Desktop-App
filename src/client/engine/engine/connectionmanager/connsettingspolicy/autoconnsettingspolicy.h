#pragma once

#include "baseconnsettingspolicy.h"
#include "engine/locationsmodel/mutablelocationinfo.h"
#include "api_responses/portmap.h"

// // manage automatic connection mode (only for API and static ips locations)
class AutoConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    AutoConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const api_responses::PortMap &portMap,
                           bool isProxyEnabled, bool skipWireguardProtocol, const QString &preferredNodeHostname);

    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    bool isCustomConfig() override;
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
    api_responses::PortMap portMap_;
    bool bIsAllFailed_;
    QString preferredNodeHostname_;

    QVector<types::ProtocolStatus> protocolStatus();
};
