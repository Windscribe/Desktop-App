#include "mergelog.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <future>

#include "paths.h"

namespace
{

// parse date from string "ddMMyy hh:mm:ss:zzz"
static QDateTime parseDateTimeFormat(const std::string &datestr)
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
    QString log;

    // Something highly suspect going on if any of these required logs are missing.
    verifyLogFileExists(paths::clientLogLocation(), log);
    verifyLogFileExists(paths::serviceLogLocation(), log);
    verifyLogFileExists(paths::installerLogLocation(), log);

    checkForCrashDumps(log);

    log += merge(paths::clientLogLocation(), paths::serviceLogLocation(), paths::serviceLogLocation(true),
                 paths::wireguardServiceLogLocation(), paths::installerLogLocation());
    return log;
}

QString MergeLog::mergePrevLogs()
{
    return merge(paths::clientLogLocation(true), paths::serviceLogLocation(), paths::serviceLogLocation(true),
                 paths::wireguardServiceLogLocation(true), "");
}

std::unique_ptr<MergeLog::ParseTaskResult> MergeLog::parseTask(const QString *filename, LineSource source)
{
    std::unique_ptr<MergeLog::ParseTaskResult> res = std::make_unique<MergeLog::ParseTaskResult>();

    if (filename->isEmpty()) {
        // Being passed an empty string is not an error and merely indicates the log likely does not exist.
        // E.g. ignore the installer log when doing a merge of the previous log files.
        return res;
    }

    QFile file(*filename);
    if (!file.exists()) {
        // We've already checked for missing required log files, so we can silently ignore this.
        return res;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        addLogEntry(source, 0, QDateTime::currentDateTime(), QString("MergeLog failed to open '%1': %2").arg(*filename, file.errorString()), res.get());
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

        addLogEntry(source, timestamp, datetime, line, res.get());
        timestamp += 1;
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
    for (const auto &it : std::as_const(clientLogRes->lines)) {
        lines.insert(it.uniqkey, it.str);
    }

    // insert other logs into the map considering the min and max date
    for (const auto &it : std::as_const(serviceLogRes->lines)) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    for (const auto &it : std::as_const(servicePrevLogRes->lines)) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    for (const auto &it : std::as_const(wgServiceLogRes->lines)) {
        if (it.datetime >= minDate && it.datetime <= maxDate)
            lines.insert(it.uniqkey, it.str);
    }

    // Include the installer log regardless of the date, it will always be at the beginning.
    for (const auto &it : std::as_const(installerLogRes->lines)) {
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

void MergeLog::addLogEntry(LineSource source, int timestamp, const QDateTime &dt, const QString &msg, ParseTaskResult *res)
{
    // We create a compound key for QMap: 44 bits for a timestamp, 2 bits
    // for the source, and 18 additional lower bits for a "timestamp" (line serial
    // number). We assume the maximum number of lines with unique timestamp never
    // exceeds 262143, which is quite reasonable (it looks infeasible to output 250k+
    // lines to a log file within 1 ms).
    const auto key = (static_cast<quint64>(dt.toMSecsSinceEpoch()) << 20)
                     | (static_cast<quint64>(source) << 18) | qMin(timestamp, 0x3ffff);

    res->lines.push_back({dt, msg, key});
    res->datasize += msg.length() + 3;
}

void MergeLog::checkForCrashDumps(QString &log)
{
#ifdef Q_OS_WIN
    QDirIterator itClientDumps(paths::clientLogFolder(), QStringList() << "*.dmp", QDir::Files);
    while (itClientDumps.hasNext()) {
        QFileInfo fi(itClientDumps.nextFileInfo());
        log += formatErrorMessage(QString("Client app crash dump '%1' found.").arg(fi.fileName()));
    }

    QDirIterator itHelperDumps(paths::serviceLogFolder(), QStringList() << "*.dmp", QDir::Files);
    while (itHelperDumps.hasNext()) {
        QFileInfo fi(itHelperDumps.nextFileInfo());
        log += formatErrorMessage(QString("Helper crash dump '%1' found.").arg(fi.fileName()));
    }
#else
    Q_UNUSED(log)
#endif
}

QString MergeLog::formatErrorMessage(const QString &error)
{
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    return QString("{\"tm\": \"%1\", \"lvl\": \"error\", \"mod\": \"mergelog\", \"msg\": \"%2\"}\n").arg(now, error);
}

void MergeLog::verifyLogFileExists(const QString& logFile, QString &log)
{
    if (!logFile.isEmpty() && !QFile::exists(logFile)) {
        log += formatErrorMessage(QString("'%1' log file is missing.").arg(logFile));
    }
}

} // namespace log_utils
