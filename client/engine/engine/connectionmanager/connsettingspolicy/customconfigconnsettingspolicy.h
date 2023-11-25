#pragma once

#include "baseconnsettingspolicy.h"
#include "engine/locationsmodel/customconfiglocationinfo.h"

// // manage manual connection mode (only for API and static ips locations)
class CustomConfigConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    explicit CustomConfigConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli);


    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    void resolveHostnames() override;
    bool hasProtocolChanged() override;

private slots:
    void onHostnamesResolved();

private:
    QSharedPointer<locationsmodel::CustomConfigLocationInfo> locationInfo_;
};
