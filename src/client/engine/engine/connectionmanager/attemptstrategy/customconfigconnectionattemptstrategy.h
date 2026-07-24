#pragma once

#include "iconnectionattemptstrategy.h"
#include "engine/locationsmodel/customconfiglocationinfo.h"

// // manage manual connection mode (only for API and static ips locations)
class CustomConfigConnectionAttemptStrategy : public IConnectionAttemptStrategy
{
    Q_OBJECT
public:
    explicit CustomConfigConnectionAttemptStrategy(QSharedPointer<locationsmodel::BaseLocationInfo> bli);


    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    void resolveHostnames() override;
    types::Protocol preResolveProtocol() const override;
    bool usesConnectTimeout() const override { return false; }
    bool surfacesTunnelTestFailure() const override { return true; }

private:
    QSharedPointer<locationsmodel::CustomConfigLocationInfo> locationInfo_;
};
