#include "networkextensionlog_mac.h"

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <time.h>
#include <xlocale.h>
#include "utils/log/categories.h"

NetworkExtensionLog_mac::NetworkExtensionLog_mac(QObject *parent) : QObject(parent)
{
}

QMap<time_t, QString> NetworkExtensionLog_mac::collectLogs(const QDateTime &start)
{
    using namespace boost::process::v1;
    QMap<time_t, QString> logs;

    ipstream pipe_stream;
    child c("/usr/bin/log", "show", "--predicate", "messageType == error and (subsystem == \"com.apple.networkextension\")",
            "--start", start.toString("yyyy-MM-dd hh:mm:ss").toStdString(), "--style", "json",
            std_out > pipe_stream);

    std::string line;
    QString answer;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        answer += QString::fromStdString(line);

    c.wait();

    QJsonParseError errCode;
    const QJsonDocument doc = QJsonDocument::fromJson(answer.toLocal8Bit(), &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isArray()) {
        qCCritical(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command";
        return logs;
    }
    for (const QJsonValue &value : doc.array()) {
        QJsonObject jsonObj = value.toObject();
        if (!jsonObj.contains("machTimestamp")) {
            qCCritical(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field machTimestamp)";
            break;
        }
        if (!jsonObj.contains("eventMessage")) {
            qCCritical(LOG_NETWORK_EXTENSION_MAC) << "Can't parse json from log command (no field eventMessage)";
            break;
        }

        quint64 t = jsonObj["machTimestamp"].toDouble();
        logs[t] = jsonObj["eventMessage"].toString();
    }

    return logs;
}
