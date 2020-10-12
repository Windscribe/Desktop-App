#ifndef AUTOCONNSETTINGSPOLICY_H
#define AUTOCONNSETTINGSPOLICY_H

#include "baseconnsettingspolicy.h"
#include "engine/locationsmodel/mutablelocationinfo.h"

// // manage automatic connection mode (only for API and static ips locations)
class AutoConnSettingsPolicy : public BaseConnSettingsPolicy
{
public:
    AutoConnSettingsPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> bli, const apiinfo::PortMap &portMap, bool isProxyEnabled);

    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    void saveCurrentSuccessfullConnectionSettings() override;
    bool isAutomaticMode() override;
    void resolveHostnames() override;

private:
    struct AttemptInfo
    {
        ProtocolType protocol;
        int portMapInd;
        bool changeNode;
    };

    QVector<AttemptInfo> attemps_;
    int curAttempt_;
    int failedIkev2Counter_;
    bool isFailedIkev2CounterAlreadyIncremented_;
    static const int MAX_IKEV2_FAILED_ATTEMPTS = 5;

    QSharedPointer<locationsmodel::MutableLocationInfo> locationInfo_;
    apiinfo::PortMap portMap_;
    bool bIsAllFailed_;
    bool bStarted_;

    QString getLogForNode(int ind) const;
    static bool sortPortMapFunction(const apiinfo::PortItem &p1, const apiinfo::PortItem &p2);
};

#endif // AUTOCONNSETTINGSPOLICY_H
