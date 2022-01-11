#ifndef MEASUREMENTCPUUSAGE_H
#define MEASUREMENTCPUUSAGE_H

#include <QObject>
#include <QHash>
#include "helper/helper_win.h"
#include "connectstatecontroller/iconnectstatecontroller.h"
#include <pdh.h>

class MeasurementCpuUsage : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementCpuUsage(QObject *parent, IHelper *helper, IConnectStateController *connectStateController);
    virtual ~MeasurementCpuUsage();

    void setEnabled(bool bEnabled);

signals:
    void detectionCpuUsageAfterConnected(QStringList processesList);

private slots:
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, ProtoTypes::ConnectError err, const LocationID &location);
    void onTimer();

private:
    Helper_win *helper_;
    PDH_HQUERY hQuery_;
    bool bEnabled_;

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
    static constexpr int TIMER_INTERVAL_FOR_CONNECTING_MODE = 1000;
    static constexpr int TIMER_INTERVAL_FOR_CONNECTED_MODE = 5000;
    QTimer timer_;

    void startInConnectingState();
    void continueInConnectedState();
    void stopInDisconnectedState();
};

#endif // MEASUREMENTCPUUSAGE_H
