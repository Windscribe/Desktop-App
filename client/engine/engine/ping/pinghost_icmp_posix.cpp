#include "pinghost_icmp_posix.h"

#include "utils/ws_assert.h"

PingHost_ICMP_posix::PingHost_ICMP_posix(QObject *parent, const QString &ip)
    : IPingHost(parent),
      ip_(ip)
{
}

void PingHost_ICMP_posix::ping()
{
    WS_ASSERT(process_ == nullptr);
    process_ = new QProcess(this);
    connect(process_, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    process_->start("ping", QStringList() << "-c" << "1" << "-W" << "2000" << ip_);
}

void PingHost_ICMP_posix::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    int timeMs = -1;
    if (exitCode == 0)
    {
        const QString strAnswer = process_->readAll();
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

    if (timeMs != -1)
        emit pingFinished(true, timeMs, ip_);
    else
        emit pingFinished(false, 0, ip_);
}

// return -1 if failed
int PingHost_ICMP_posix::extractTimeMs(const QString &str)
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
