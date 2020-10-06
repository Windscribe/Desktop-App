#ifndef MANUALCONNSETTINGSPOLICY_H
#define MANUALCONNSETTINGSPOLICY_H

#include "baseconnsettingspolicy.h"
#include "engine/types/connectionsettings.h"
#include "engine/locationsmodel/mutablelocationinfo.h"

// // manage manual connection mode (only for API and static ips locations)
class ManualConnSettingsPolicy : public BaseConnSettingsPolicy
{
public:
    ManualConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const ConnectionSettings &connectionSettings,
                             const apiinfo::PortMap &portMap);


    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    void saveCurrentSuccessfullConnectionSettings() override;
    bool isAutomaticMode() override;

private:
    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    apiinfo::PortMap portMap_;
    ConnectionSettings connectionSettings_;
    int failedManualModeCounter_;

    QString getLogForNode(int ind) const;
};

#endif // MANUALCONNSETTINGSPOLICY_H
