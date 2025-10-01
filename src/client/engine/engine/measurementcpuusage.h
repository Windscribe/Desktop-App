#pragma once

#include <QHash>
#include <QObject>

#include <pdh.h>

#include "connectstatecontroller/iconnectstatecontroller.h"
#include "helper/helper.h"

class MeasurementCpuUsage : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementCpuUsage(QObject *parent, Helper *helper, IConnectStateController *connectStateController);
    virtual ~MeasurementCpuUsage();

    void setEnabled(bool bEnabled);

signals:
    void detectionCpuUsageAfterConnected(QStringList processesList);

private slots:
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);
    void onTimer();

private:
    Helper* const helper_;
    PDH_HQUERY hQuery_ = NULL;
    bool bEnabled_ = false;

    struct UsageData
    {
        bool isValid;
        double value;

        UsageData() : isValid(false), value(0) {}
    };

    struct CounterDescr
    {
        PDH_HCOUNTER counter_;
        bool bUsed;
        UsageData cpuUsageInDisconnectedState[2];    // 2 latest values of CPU usage in disconnected state

        void putNextCpuUsageInDisconnectedState(double value)
        {
            if (!cpuUsageInDisconnectedState[0].isValid)
            {
                cpuUsageInDisconnectedState[0].value = value;
                cpuUsageInDisconnectedState[0].isValid = true;
            }
            else if (!cpuUsageInDisconnectedState[1].isValid)
            {
                cpuUsageInDisconnectedState[1].value = value;
                cpuUsageInDisconnectedState[1].isValid = true;
            }
            else
            {
                cpuUsageInDisconnectedState[0] = cpuUsageInDisconnectedState[1];
                cpuUsageInDisconnectedState[1].value = value;
                cpuUsageInDisconnectedState[1].isValid = true;
            }
        }
    };

    QHash<QString, CounterDescr> counters_;
    static constexpr int kTimeoutForConnectingMode = 1000;
    static constexpr int kTimeoutForConnectedMode = 5000;
    QTimer timer_;

    void clearPerformanceCounters();
    bool getPerformanceCounters();

    void continueInConnectedState();
    void startInConnectingState();
    void stopInDisconnectedState();
};
