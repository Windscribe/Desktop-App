#include "pinghost_icmp_mac.h"
#include "utils/ipvalidation.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "icmp_header.h"
#include "ipv4_header.h"

PingHost_ICMP_mac::PingHost_ICMP_mac(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    connectStateController_(stateController)
{

}

PingHost_ICMP_mac::~PingHost_ICMP_mac()
{
    clearPings();
}

void PingHost_ICMP_mac::addHostForPing(const QString &ip)
{
    QMutexLocker locker(&mutex_);
    if (!hostAlreadyPingingOrInWaitingQueue(ip))
    {
        waitingPingsQueue_.enqueue(ip);
        processNextPings();
    }
}

void PingHost_ICMP_mac::clearPings()
{   
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

void PingHost_ICMP_mac::setProxySettings(const ProxySettings &proxySettings)
{
    //todo
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
    int timeMs = -1;
    QProcess *process = (QProcess *)sender();
    if (exitCode == 0)
    {
        const QString strAnswer = process->readAll();
        const QStringList lines = strAnswer.split("\n");
        for (const QString &line : lines)
        {
            if (line.contains("icmp_seq=0"))
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
                    Q_ASSERT(false);
                }
                break;
            }
        }
    }
    else if (exitCode != 2)
    {
        qCDebug(LOG_PING) << "ping utility return not 0 exitCode:" << exitCode;
    }

    QString ip = process->property("ip").toString();
    auto it = pingingHosts_.find(ip);
    if (it != pingingHosts_.end())
    {
        PingInfo *pingInfo = it.value();
        bool bFromDisconnectedState = pingInfo->process->property("fromDisconnectedState").toBool();
        pingInfo->process->deleteLater();
        pingingHosts_.remove(ip);
        delete pingInfo;
        if (timeMs != -1)
        {
            emit pingFinished(true, timeMs, ip, bFromDisconnectedState);
        }
        else
        {
            emit pingFinished(false, 0, ip, bFromDisconnectedState);
        }
    }
    waitingPingsQueue_.removeAll(ip);
    processNextPings();
}


bool PingHost_ICMP_mac::hostAlreadyPingingOrInWaitingQueue(const QString &ip)
{
    return pingingHosts_.find(ip) != pingingHosts_.end() || waitingPingsQueue_.indexOf(ip) != -1;
}

void PingHost_ICMP_mac::processNextPings()
{
    if (pingingHosts_.count() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty())
    {
        QString ip = waitingPingsQueue_.dequeue();
        Q_ASSERT(IpValidation::instance().isIp(ip));


        PingInfo *pingInfo = new PingInfo();
        pingInfo->ip = ip;

        pingInfo->process = new QProcess(this);
        connect(pingInfo->process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished(int,QProcess::ExitStatus)));

        if (connectStateController_)
        {
            pingInfo->process->setProperty("fromDisconnectedState", connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED || connectStateController_->currentState() == CONNECT_STATE_CONNECTING);
        }
        else
        {
            pingInfo->process->setProperty("fromDisconnectedState", true);
        }

        pingInfo->process->setProperty("ip", ip);

        pingingHosts_[ip] = pingInfo;
        pingInfo->process->start("ping", QStringList() << "-c" << "1" << "-W" << "2000" << ip);
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


