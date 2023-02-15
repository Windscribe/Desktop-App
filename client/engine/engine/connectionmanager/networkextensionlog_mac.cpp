#include "networkextensionlog_mac.h"

#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <time.h>
#include <xlocale.h>
#include "utils/logger.h"

NetworkExtensionLog_mac::NetworkExtensionLog_mac(QObject *parent) : QObject(parent),
    isInitialTimestampInitialized_(false)
{
    // detect initial timestamp for latest record in log
    collectNext();
    if (!logs_.empty())
    {
        initialTimestamp_ = (std::prev(logs_.end())).key();
        isInitialTimestampInitialized_ = true;
        logs_.clear();
    }
}

QMap<time_t, QString> NetworkExtensionLog_mac::collectNext()
{
    QMap<time_t, QString> nextLogs;
    //calc minutes for pass to log command
    uint minutes;

    if (!lastTimeCollected_.isNull())
    {
        qint64 secs = lastTimeCollected_.secsTo(QDateTime::currentDateTime());
        minutes = (secs + 30) / 60 + 2;    // +30 secs - extra protective interval
    }
    else
    {
        minutes = 5;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QStringList pars;
    pars << "show";
    //pars << "--predicate" << "(messageType=\"error\" && (subsystem == \"com.apple.networkextension\" || process == \"neagent\"))";
    pars << "--predicate" << "messageType == error and (subsystem == \"com.apple.networkextension\")";
    pars << "--last" << QString::number(minutes) + "m";
    pars << "--style" << "json";

    process.start("log", pars);
    if (process.waitForFinished(10000))
    {

        QJsonParseError errCode;
        const QJsonDocument doc = QJsonDocument::fromJson(process.readAll(), &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isArray())
        {
            qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command";
            return nextLogs;
        }
        for (const QJsonValue &value : doc.array())
        {
            QJsonObject jsonObj = value.toObject();
            if (!jsonObj.contains("machTimestamp"))
            {
                qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field machTimestamp)";
                return nextLogs;
            }
            if (!jsonObj.contains("eventMessage"))
            {
                qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field eventMessage)";
                return nextLogs;
            }

            quint64 t = jsonObj["machTimestamp"].toDouble();
            //struct tm sometime;
            //const char *formatString = "%Y-%m-%d %H:%M:%S %z";
            //strptime_l(timestamp.toStdString().c_str(), formatString, &sometime, NULL);
            //time_t t = mktime(&sometime);

            if (!logs_.contains(t))
            {
                if (isInitialTimestampInitialized_)
                {
                    if (t > initialTimestamp_)
                    {
                        logs_[t] = jsonObj["eventMessage"].toString();
                        nextLogs[t] = jsonObj["eventMessage"].toString();
                    }
                }
                else
                {
                    logs_[t] = jsonObj["eventMessage"].toString();
                    nextLogs[t] = jsonObj["eventMessage"].toString();
                }
            }
        }

        lastTimeCollected_ = QDateTime::currentDateTime();
    }
    return nextLogs;
}
