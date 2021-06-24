#include "measurementcpuusage.h"
#include <QElapsedTimer>
#include "Utils/logger.h"

MeasurementCpuUsage::MeasurementCpuUsage(QObject *parent, IHelper *helper, IConnectStateController *connectStateController) : QObject(parent),
    bEnabled_(false)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_);
    connect(connectStateController, SIGNAL(stateChanged(CONNECT_STATE, DISCONNECT_REASON, CONNECTION_ERROR, LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE, DISCONNECT_REASON, CONNECTION_ERROR, LocationID)));
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));

    if (PdhOpenQuery(NULL, 0, &hQuery_) == ERROR_SUCCESS)
    {

    }
    else
    {
        hQuery_ = NULL;
    }
}

MeasurementCpuUsage::~MeasurementCpuUsage()
{
    timer_.stop();
    auto it = counters_.begin();
    while (it != counters_.end())
    {
        PdhRemoveCounter(it.value().counter_);
        it = counters_.erase(it);
    }
    counters_.clear();
    if (hQuery_)
    {
        PdhCloseQuery(hQuery_);
    }
}

void MeasurementCpuUsage::setEnabled(bool bEnabled)
{
    bEnabled_ = bEnabled;
    if (!bEnabled)
    {
        stopInDisconnectedState();
    }
}

void MeasurementCpuUsage::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON /*reason*/, CONNECTION_ERROR /*err*/, const LocationID & /*location*/)
{
    if (bEnabled_)
    {
        if (state == CONNECT_STATE_DISCONNECTED)
        {
            stopInDisconnectedState();
        }
        else if (state == CONNECT_STATE_CONNECTING)
        {
            startInConnectingState();
        }
        else if (state == CONNECT_STATE_CONNECTED)
        {
            continueInConnectedState();
        }
    }
}

void MeasurementCpuUsage::startInConnectingState()
{
    qDebug() << "MeasurementCpuUsage started";

    const QStringList processesList = helper_->getProcessesList();

    for(auto it = counters_.begin(); it != counters_.end(); ++it)
    {
        it.value().bUsed = false;
    }

    for (const QString &processName : processesList)
    {
        auto it = counters_.find(processName);
        if (it == counters_.end())
        {
            PDH_HCOUNTER hCounter;
            QString counterPath =  QString("\\Process(%1)\\% Processor Time").arg(processName);
            if (PdhAddEnglishCounter(hQuery_, counterPath.toStdWString().c_str(), 0, &hCounter) == ERROR_SUCCESS)
            {
                CounterDescr cd;
                cd.counter_ = hCounter;
                cd.bUsed = true;
                counters_[processName] = cd;
            }
            else
            {
                qCDebug(LOG_BASIC) << "updateProcessesList failed add counter for process =" << processName;
            }
        }
        else
        {
            it.value().bUsed = true;
        }
    }

    // remove unused processes and counters
    auto it = counters_.begin();
    while (it != counters_.end())
    {
        if (!it.value().bUsed)
        {
            PdhRemoveCounter(it.value().counter_);
            it = counters_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    PdhCollectQueryData(hQuery_);
    timer_.setProperty("startedInDisconnectedState", true);
    timer_.start(TIMER_INTERVAL_FOR_CONNECTING_MODE);
}

void MeasurementCpuUsage::continueInConnectedState()
{
    Q_ASSERT(timer_.isActive());
    qDebug() << "MeasurementCpuUsage detect CPU usage in connected state";
    PdhCollectQueryData(hQuery_);
    timer_.setProperty("startedInDisconnectedState", false);
    timer_.start(TIMER_INTERVAL_FOR_CONNECTED_MODE);
}

void MeasurementCpuUsage::stopInDisconnectedState()
{
    qDebug() << "MeasurementCpuUsage stopped";
    timer_.stop();
    auto it = counters_.begin();
    while (it != counters_.end())
    {
        PdhRemoveCounter(it.value().counter_);
        it = counters_.erase(it);
    }
    counters_.clear();
}

void MeasurementCpuUsage::onTimer()
{
    PdhCollectQueryData(hQuery_);

    QStringList processesForPopup;
    bool isTimerStartedInDisconnectedState = timer_.property("startedInDisconnectedState").toBool();

    for (auto it = counters_.begin(); it != counters_.end(); ++it)
    {
        DWORD counterType;
        PDH_FMT_COUNTERVALUE value;
        PDH_STATUS status = PdhGetFormattedCounterValue(it.value().counter_, PDH_FMT_DOUBLE, &counterType, &value);
        if (status == ERROR_SUCCESS)
        {
            if (isTimerStartedInDisconnectedState)
            {
                it.value().putNextCpuUsageInDisconnectedState(value.doubleValue);
            }
            else
            {
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
                const double marginValueInConnectedState = 80.0;   // 80 % CPU usage
                const double marginValueInDisconnectedState = 60.0;   // 60 % CPU usage
                if (value.doubleValue > marginValueInConnectedState)
                {
                    if (!it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid)
                    {
                        processesForPopup << it.key();
                    }
                    else if (it.value().cpuUsageInDisconnectedState[0].isValid && !it.value().cpuUsageInDisconnectedState[1].isValid)
                    {
                        if (it.value().cpuUsageInDisconnectedState[0].value < marginValueInDisconnectedState)
                        {
                            processesForPopup << it.key();
                        }
                    }
                    else if (it.value().cpuUsageInDisconnectedState[0].isValid && it.value().cpuUsageInDisconnectedState[1].isValid)
                    {
                        if (it.value().cpuUsageInDisconnectedState[0].value < marginValueInDisconnectedState || it.value().cpuUsageInDisconnectedState[1].value < marginValueInDisconnectedState)
                        {
                            processesForPopup << it.key();
                        }
                    }
                }
            }
        }
        else
        {
            //qDebug() << "MeasurementCpuUsage::onTimer(),  failed get CPU usage value for process: " << it.key();
        }
    }

    if (isTimerStartedInDisconnectedState)
    {
        // check for new processes
        bool bAddedNewProcesses = false;
        const QStringList processesList = helper_->getProcessesList();

        for(auto it = counters_.begin(); it != counters_.end(); ++it)
        {
            it.value().bUsed = false;
        }

        for (const QString &processName : processesList)
        {
            auto it = counters_.find(processName);
            if (it == counters_.end())
            {
                PDH_HCOUNTER hCounter;
                QString counterPath =  QString("\\Process(%1)\\% Processor Time").arg(processName);
                if (PdhAddEnglishCounter(hQuery_, counterPath.toStdWString().c_str(), 0, &hCounter) == ERROR_SUCCESS)
                {
                    CounterDescr cd;
                    cd.counter_ = hCounter;
                    cd.bUsed = true;
                    counters_[processName] = cd;
                    bAddedNewProcesses = true;
                }
                else
                {
                    //qCDebug(LOG_BASIC) << "updateProcessesList failed add counter for process =" << processName;
                }
            }
            else
            {
                it.value().bUsed = true;
            }
        }

        // remove unused processes and counters
        auto it = counters_.begin();
        while (it != counters_.end())
        {
            if (!it.value().bUsed)
            {
                PdhRemoveCounter(it.value().counter_);
                it = counters_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (bAddedNewProcesses)
        {
            //qCDebug(LOG_BASIC) << "New processes added";
            PdhCollectQueryData(hQuery_);
        }
    }
    else //if (isTimerStartedInDisconnectedState)
    {
        stopInDisconnectedState();

        if (!processesForPopup.isEmpty())
        {
            std::sort(processesForPopup.begin(), processesForPopup.end());
            emit detectionCpuUsageAfterConnected(processesForPopup);
        }
    }
}
