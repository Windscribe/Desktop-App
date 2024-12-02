#include "mergelog.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <future>

#include "paths.h"

namespace
{

// parse date from string "ddMMyy hh:mm:ss:zzz"
QDateTime parseDateTimeFormat(const std::string &datestr)
{
    int dd,MM,yy;
    int hh, mm, ss, zzz;
    if (sscanf(datestr.c_str(),"{\"tm\": \"%04d-%02d-%02d %02d:%02d:%02d.%03d", &yy, &MM, &dd, &hh, &mm, &ss, &zzz) == 7) {
        return QDateTime(QDate(yy, MM, dd), QTime(hh, mm, ss, zzz));
    }

    return QDateTime();
}

}  // namespace

namespace log_utils {

QString MergeLog::mergeLogs()
{
    return merge(paths::clientLogLocation(), paths::serviceLogLocation(), paths::serviceLogLocation(true),
                 paths::wireguardServiceLogLocation(), paths::installerLogLocation());
}

QString MergeLog::mergePrevLogs()
{
    return merge(paths::clientLogLocation(true), paths::serviceLogLocation(), paths::serviceLogLocation(true),
                 paths::wireguardServiceLogLocation(true), "");
}

std::unique_ptr<MergeLog::ParseTaskResult> MergeLog::parseTask(const QString *filename, LineSource source)
{
    std::unique_ptr<MergeLog::ParseTaskResult> res = std::make_unique<MergeLog::ParseTaskResult>();

    if (filename->isEmpty())
        return res;  // return empty list of lines

    QFile file(*filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return res;
    }

    QTextStream textStream(&file);
    // If file is larger than 10MiB, just take the last 10MiB
    int64_t filelen = file.size();
    if (filelen > 10000000) {
        file.seek(filelen - 10000000);
    }

    // Pre-allocation for some optimization. Let's take an average of 250 bytes per line.
    res->lines.reserve(filelen / 250);

    QDateTime prevDateTime;
    int timestamp = 0;

    while (!textStream.atEnd())
    {
        QString line = textStream.readLine();

        // Simple json validation
        if (line[0] != '{' || line[line.length() - 1] != '}')
            continue;

        const auto datestr = line.sliced(0, 32).toStdString();
        const auto datetime = parseDateTimeFormat(datestr);
        if (!datetime.isValid())
            continue;

        if (prevDateTime != datetime) {
            prevDateTime = datetime;
            timestamp = 0;
        }

        // We create a compound key for QMap: 44 bits for a timestamp, 2 bits
        // for the source, and 18 additional lower bits for a "timestamp" (line serial
        // number). We assume the maximum number of lines with unique timestamp never
        // exceeds 262143, which is quite reasonable (it looks infeasible to output 250k+
        // lines to a log file within 1 ms).
        const auto key = (static_cast<quint64>(datetime.toMSecsSinceEpoch()) << 20)
                       | (static_cast<quint64>(source) << 18) | qMin(timestamp++, 0x3ffff);

        res->lines.push_back({datetime, line, key});
        res->datasize += line.length() + 3;
    }
    return res;
}


QString MergeLog::merge(const QString &guiLogFilename, const QString &serviceLogFilename, const QString &wireguardServiceLogFilename,
                        const QString &servicePrevLogFilename, const QString &installerLogFilename)
{
    // Do parallel parsing for speed
    auto futureClientLog = std::async(MergeLog::parseTask, &guiLogFilename, LineSource::CLIENT);
    auto futureServiceLog = std::async(MergeLog::parseTask, &serviceLogFilename, LineSource::SERVICE);
    auto futureServicePrevLog = std::async(MergeLog::parseTask, &servicePrevLogFilename, LineSource::SERVICE);
    auto futureWGServiceLog = std::async(MergeLog::parseTask, &wireguardServiceLogFilename, LineSource::WIREGUARD_SERVICE);
    auto futureInstallerLog = std::async(MergeLog::parseTask, &installerLogFilename, LineSource::INSTALLER);

    // Get results
    auto clientLogRes = futureClientLog.get();
    auto serviceLogRes = futureServiceLog.get();
    auto servicePrevLogRes = futureServicePrevLog.get();
    auto wgServiceLogRes = futureWGServiceLog.get();
    auto installerLogRes = futureInstallerLog.get();

    // Take the limits of the date range from the client's log
    QDateTime minDate, maxDate;
    if (clientLogRes->lines.size() > 1) {
        minDate = clientLogRes->lines.front().datetime;
        maxDate = clientLogRes->lines.back().datetime;
    } else {
        return "Empty";
    }

    QMap<quint64, QString> lines;

    // insert client's log into the map
    for (const auto &it : clientLogRes->lines) {
        lines.insert(it.uniqkey, it.str);
    }

    // insert other logs into the map considering the min and max date
    for (const auto &it : serviceLogRes->lines) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    for (const auto &it : servicePrevLogRes->lines) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    for (const auto &it : wgServiceLogRes->lines) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    // Include the installer log regardless of the date, it will always be at the beginning.
    for (const auto &it : installerLogRes->lines) {
        lines.insert(it.uniqkey, it.str);
    }

    QString result;
    // Pre-allocation for some optimization.
    result.reserve(clientLogRes->datasize + serviceLogRes->datasize + wgServiceLogRes->datasize + installerLogRes->datasize);

    // cut out the part of the log if the count of lines  exceeds MAX_COUNT_OF_LINES (keep 10% begin and 90% end of log)
    int cutCount = 0;
    int cutBeginInd = 0;
    int cutEndInd = lines.count();
    if (lines.count() > MAX_COUNT_OF_LINES) {
        cutCount = lines.count() - MAX_COUNT_OF_LINES;
        cutBeginInd = MAX_COUNT_OF_LINES / 10;
        cutEndInd = lines.count() - MAX_COUNT_OF_LINES * 0.9;
    }

    int ind = 0;
    for (auto it = lines.constBegin(); it != lines.constEnd(); ++it) {
        // cut out middle
        if (cutCount == 0 || ind < cutBeginInd || ind > cutEndInd) {
            result.append(it.value());
            result.append("\n");
        }
        ind++;
    }

    return result;
}

} // namespace log_utils
