#pragma once

#include <QDateTime>
#include <QMutex>
#include <QString>

namespace log_utils {

// merge logs files log_gui.txt, windscribeservice.log, and WireguardServiceLog.txt (Windows only) to one,
// cutting out the middle of the log if the count of lines exceeds MAX_COUNT_OF_LINES
class MergeLog
{
public:
    static QString mergeLogs();
    static QString mergePrevLogs();

private:
    static constexpr int MAX_COUNT_OF_LINES = 100000;
    enum class LineSource { CLIENT, SERVICE, WIREGUARD_SERVICE, NUM_LINE_SOURCES, INSTALLER };

    struct LineInfo {
        QDateTime datetime;
        QString str;
        quint64 uniqkey;
    };

    struct ParseTaskResult {
        QVector<LineInfo> lines;
        quint32 datasize = 0;
    };

    static QString merge(const QString &guiLogFilename, const QString &serviceLogFilename, const QString &servicePrevLogFilename,
                         const QString &wireguardServiceLogFilename, const QString &installerLogFilename);

    static std::unique_ptr<ParseTaskResult> parseTask(const QString *filename, LineSource source);

    static void addLogEntry(LineSource source, int timestamp, const QDateTime &dt, const QString &msg, MergeLog::ParseTaskResult *res);
    static void checkForCrashDumps(QString &log);
    static QString formatErrorMessage(const QString &error);
    static void verifyLogFileExists(const QString& logFile, QString &log);
};

}  // namespace log_utils
