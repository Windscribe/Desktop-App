#include "measurementcpuusage.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <pdhmsg.h>

#include "engine/helper/helper_win.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

MeasurementCpuUsage::MeasurementCpuUsage(QObject *parent, IHelper *helper, IConnectStateController *connectStateController)
    : QObject(parent),
      helper_(dynamic_cast<Helper_win*>(helper))
{
    WS_ASSERT(helper_);

    PDH_STATUS status = PdhOpenQuery(NULL, 0, &hQuery_);
    if (status == ERROR_SUCCESS) {
        connect(connectStateController, &IConnectStateController::stateChanged, this, &MeasurementCpuUsage::onConnectStateChanged);
        connect(&timer_, &QTimer::timeout, this, &MeasurementCpuUsage::onTimer);
    }
    else {
        qCDebug(LOG_BASIC) << "MeasurementCpuUsage - PdhOpenQuery failed: " << status;
        hQuery_ = NULL;
    }
}

MeasurementCpuUsage::~MeasurementCpuUsage()
{
    timer_.stop();
    clearPerformanceCounters();

    if (hQuery_) {
        PdhCloseQuery(hQuery_);
    }
}

void MeasurementCpuUsage::setEnabled(bool bEnabled)
{
    bEnabled_ = bEnabled;
    if (!bEnabled) {
        stopInDisconnectedState();
    }
}

void MeasurementCpuUsage::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason,
                                                CONNECT_ERROR err, const LocationID &location)
{
    Q_UNUSED(reason)
    Q_UNUSED(err)
    Q_UNUSED(location)

    if (bEnabled_) {
        if (state == CONNECT_STATE_DISCONNECTED) {
            stopInDisconnectedState();
        }
        else if (state == CONNECT_STATE_CONNECTING) {
            startInConnectingState();
        }
        else if (state == CONNECT_STATE_CONNECTED) {
            continueInConnectedState();
        }
    }
}

void MeasurementCpuUsage::startInConnectingState()
{
    qDebug() << "MeasurementCpuUsage started";

    getPerformanceCounters();
    PdhCollectQueryData(hQuery_);
    timer_.setProperty("startedInDisconnectedState", true);
    timer_.start(kTimeoutForConnectingMode);
}

void MeasurementCpuUsage::continueInConnectedState()
{
    WS_ASSERT(timer_.isActive());
    qDebug() << "MeasurementCpuUsage detect CPU usage in connected state";
    PdhCollectQueryData(hQuery_);
    timer_.setProperty("startedInDisconnectedState", false);
    timer_.start(kTimeoutForConnectedMode);
}

void MeasurementCpuUsage::stopInDisconnectedState()
{
    qDebug() << "MeasurementCpuUsage stopped";
    timer_.stop();
    clearPerformanceCounters();
}

void MeasurementCpuUsage::onTimer()
{
    PdhCollectQueryData(hQuery_);

    QStringList processesForPopup;
    bool isTimerStartedInDisconnectedState = timer_.property("startedInDisconnectedState").toBool();

    for (auto it = counters_.begin(); it != counters_.end(); ++it) {
        DWORD counterType;
        PDH_FMT_COUNTERVALUE value;
        PDH_STATUS status = PdhGetFormattedCounterValue(it.value().counter_, PDH_FMT_DOUBLE, &counterType, &value);
        if (status == ERROR_SUCCESS) {
            if (isTimerStartedInDisconnectedState) {
                it.value().putNextCpuUsageInDisconnectedState(value.doubleValue);
            }
            else {
                // for extended logs
                /*QString disconnectedValues;
                if (!it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid)
                {
                    disconnectedValues = "No values for disconnected";
                }
                else if (it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid)
                {
                    disconnectedValues = QString::number(it.value().cpuUsageInDisconnectedState[0].value);
                }
                else if (it.value().cpuUsageInDisconnectedState[0].isValid && it.value().cpuUsageInDisconnectedState[1].isValid)
                {
                    disconnectedValues = QString::number(it.value().cpuUsageInDisconnectedState[0].value) + "; " + QString::number(it.value().cpuUsageInDisconnectedState[1].value);
                }

                qDebug() << "Process" << it.key() << " disconnected: " << disconnectedValues << "\t connected:" << value.doubleValue;*/

                // calc for popup message
                const double kMarginValueInConnectedState = 80.0;   // 80 % CPU usage
                const double kMarginValueInDisconnectedState = 60.0;   // 60 % CPU usage

                if (value.doubleValue > kMarginValueInConnectedState) {
                    if (!it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid) {
                        processesForPopup << it.key();
                    }
                    else if (it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid) {
                        if (it.value().cpuUsageInDisconnectedState[0].value < kMarginValueInDisconnectedState) {
                            processesForPopup << it.key();
                        }
                    }
                    else if (it.value().cpuUsageInDisconnectedState[0].isValid && it.value().cpuUsageInDisconnectedState[1].isValid) {
                        if (it.value().cpuUsageInDisconnectedState[0].value < kMarginValueInDisconnectedState || it.value().cpuUsageInDisconnectedState[1].value < kMarginValueInDisconnectedState) {
                            processesForPopup << it.key();
                        }
                    }
                }
            }
        }
        else {
            //qDebug() << "MeasurementCpuUsage::onTimer(),  failed get CPU usage value for process: " << it.key();
        }
    }

    if (isTimerStartedInDisconnectedState) {
        if (getPerformanceCounters()) {
            PdhCollectQueryData(hQuery_);
        }
    }
    else {
        stopInDisconnectedState();

        if (!processesForPopup.isEmpty()) {
            std::sort(processesForPopup.begin(), processesForPopup.end());
            emit detectionCpuUsageAfterConnected(processesForPopup);
        }
    }
}

bool MeasurementCpuUsage::getPerformanceCounters()
{
    bool bAddedNewProcesses = false;
    const QStringList processesList = helper_->getProcessesList();

    for (auto it = counters_.begin(); it != counters_.end(); ++it) {
        it.value().bUsed = false;
    }

    QRegularExpression exclude("svchost|svchost#\\d+");

    for (const QString &processName : processesList) {
        QRegularExpressionMatch match = exclude.match(processName);
        if (!match.hasMatch()) {
            auto it = counters_.find(processName);
            if (it == counters_.end()) {
                PDH_HCOUNTER hCounter = NULL;
                QString counterPath =  QString("\\Process(%1)\\% Processor Time").arg(processName);
                PDH_STATUS status = PdhAddEnglishCounter(hQuery_, counterPath.toStdWString().c_str(), 0, &hCounter);
                if (status == ERROR_SUCCESS) {
                    CounterDescr cd;
                    cd.counter_ = hCounter;
                    cd.bUsed = true;
                    counters_[processName] = cd;
                    bAddedNewProcesses = true;
                }
                else {
                    if (hCounter != NULL) {
                        PdhRemoveCounter(hCounter);
                    }
                    if (IsErrorSeverity(status)) {
                        if (status != PDH_CSTATUS_NO_OBJECT && status != PDH_CSTATUS_NO_COUNTER) {
                            qCDebug(LOG_BASIC) << "MeasurementCpuUsage::getPerformanceCounters failed to add counter for process" << processName << status;
                        }
                    }
                }
            }
            else {
                it.value().bUsed = true;
            }
        }
    }

    // remove unused processes and counters
    auto it = counters_.begin();
    while (it != counters_.end()) {
        if (!it.value().bUsed) {
            PdhRemoveCounter(it.value().counter_);
            it = counters_.erase(it);
        }
        else {
            ++it;
        }
    }

    return bAddedNewProcesses;
}

void MeasurementCpuUsage::clearPerformanceCounters()
{
    auto it = counters_.begin();
    while (it != counters_.end()) {
        PdhRemoveCounter(it.value().counter_);
        it = counters_.erase(it);
    }
    counters_.clear();
}
