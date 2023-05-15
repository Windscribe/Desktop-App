#include "pinghost_icmp_mac.h"

#include "utils/ipvalidation.h"
#include "utils/logger.h"
#include "utils/ws_assert.h"

PingHost_ICMP_mac::PingHost_ICMP_mac(QObject *parent, IConnectStateController *stateController)
    : QObject(parent),
      connectStateController_(stateController)
{
}

PingHost_ICMP_mac::~PingHost_ICMP_mac()
{
    clearPings();
}

void PingHost_ICMP_mac::addHostForPing(const QString &id, const QString &ip)
{
    QMutexLocker locker(&mutex_);
    if (!hostAlreadyPingingOrInWaitingQueue(id))
    {
        QueueJob job;
        job.id = id;
        job.ip = ip;
        waitingPingsQueue_.enqueue(job);
        processNextPings();
    }
}

void PingHost_ICMP_mac::clearPings()
{
    QMutexLocker locker(&mutex_);
    for (QMap<QString, PingInfo *>::iterator it = pingingHosts_.begin(); it != pingingHosts_.end(); ++it)
    {
        it.value()->process->blockSignals(true);
        it.value()->process->kill();
        it.value()->process->waitForFinished();
        delete it.value()->process;
        delete it.value();
    }

    pingingHosts_.clear();
    waitingPingsQueue_.clear();
}

void PingHost_ICMP_mac::setProxySettings(const types::ProxySettings &proxySettings)
{
    //todo
    Q_UNUSED(proxySettings);
}

void PingHost_ICMP_mac::disableProxy()
{
    //todo
}

void PingHost_ICMP_mac::enableProxy()
{
    //todo
}

void PingHost_ICMP_mac::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    int timeMs = -1;
    QProcess *process = (QProcess *)sender();
    if (exitCode == 0)
    {
        const QString strAnswer = process->readAll();
        const QStringList lines = strAnswer.split("\n");
        for (const QString &line : lines)
        {
            // macOS ping command produces icmp_seq=0, Linux produces icmp_seq=1.
            if (line.contains("icmp_seq=0") || line.contains("icmp_seq=1"))
            {
                int ind = line.indexOf("time=");
                if (ind != -1)
                {
                    QString strTime = line.mid(ind, line.length() - ind);
                    timeMs = extractTimeMs(strTime);
                }
                else
                {
                    qCDebug(LOG_PING) << "Something incorrect in ping utility output:" << line;
                    WS_ASSERT(false);
                }
                break;
            }
        }
    }
    else if (exitCode != 2)
    {
        qCDebug(LOG_PING) << "ping utility return not 0 exitCode:" << exitCode;
    }

    QString id = process->property("id").toString();

    QMutexLocker locker(&mutex_);
    auto it = pingingHosts_.find(id);
    if (it != pingingHosts_.end())
    {
        PingInfo *pingInfo = it.value();
        bool bFromDisconnectedState = pingInfo->process->property("fromDisconnectedState").toBool();
        pingInfo->process->deleteLater();
        pingingHosts_.remove(id);
        delete pingInfo;
        if (timeMs != -1)
        {
            emit pingFinished(true, timeMs, id, bFromDisconnectedState);
        }
        else
        {
            emit pingFinished(false, 0, id, bFromDisconnectedState);
        }
    }

    removeFromQueue(id);
    processNextPings();
}

bool PingHost_ICMP_mac::hostAlreadyPingingOrInWaitingQueue(const QString &id)
{
    if (pingingHosts_.find(id) != pingingHosts_.end())
        return true;

    for (const auto &it : waitingPingsQueue_)
        if (it.id == id)
            return true;

    return false;
}

void PingHost_ICMP_mac::processNextPings()
{
    if (pingingHosts_.count() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty())
    {
        QueueJob job = waitingPingsQueue_.dequeue();
        WS_ASSERT(IpValidation::isIp(job.ip));

        PingInfo *pingInfo = new PingInfo();
        pingInfo->ip = job.ip;

        pingInfo->process = new QProcess(this);
        connect(pingInfo->process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished(int,QProcess::ExitStatus)));

        if (connectStateController_) {
            pingInfo->process->setProperty("fromDisconnectedState", connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED);
        }
        else {
            pingInfo->process->setProperty("fromDisconnectedState", true);
        }

        pingInfo->process->setProperty("id", job.id);

        pingingHosts_[job.id] = pingInfo;
        pingInfo->process->start("ping", QStringList() << "-c" << "1" << "-W" << "2000" << job.ip);
    }
}

// return -1 if failed
int PingHost_ICMP_mac::extractTimeMs(const QString &str)
{
    int ind = str.indexOf('=');
    if (ind != -1)
    {
        QString val;
        ind++;
        while (ind < str.length() && !str[ind].isSpace())
        {
            val += str[ind];
            ind++;
        }

        bool bOk;
        double d = val.toDouble(&bOk);
        if (bOk)
        {
            return (int)d;
        }
    }
    return -1;
}

void PingHost_ICMP_mac::removeFromQueue(const QString &id)
{
    QMutableListIterator<QueueJob> i(waitingPingsQueue_);
    while (i.hasNext()) {
        if (i.next().id == id)
            i.remove();
    }
}


