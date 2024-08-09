#include "networkextensionlog_mac.h"

#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <time.h>
#include <xlocale.h>
#include "utils/logger.h"

NetworkExtensionLog_mac::NetworkExtensionLog_mac(QObject *parent) : QObject(parent)
{
}

QMap<time_t, QString> NetworkExtensionLog_mac::collectLogs(const QDateTime &start)
{
    QMap<time_t, QString> logs;

    QStringList pars;
    pars << "show";
    pars << "--predicate" << "messageType == error and (subsystem == \"com.apple.networkextension\")";
    pars << "--start" << start.toString("yyyy-MM-dd hh:mm:ss");
    pars << "--style" << "json";

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("log", pars);

    if (process.waitForFinished(10000)) {
        QJsonParseError errCode;
        const QJsonDocument doc = QJsonDocument::fromJson(process.readAll(), &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isArray()) {
            qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command";
            return logs;
        }
        for (const QJsonValue &value : doc.array()) {
            QJsonObject jsonObj = value.toObject();
            if (!jsonObj.contains("machTimestamp")) {
                qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field machTimestamp)";
                break;
            }
            if (!jsonObj.contains("eventMessage")) {
                qCDebug(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field eventMessage)";
                break;
            }

            quint64 t = jsonObj["machTimestamp"].toDouble();
            logs[t] = jsonObj["eventMessage"].toString();
        }
    }
    else {
        qCDebug(LOG_NETWORK_EXTENSION_MAC) << "log read failed or timed out";
    }

    return logs;
}
