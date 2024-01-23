#pragma once

#include <QDateTime>
#include <QMutex>
#include <QString>

// merge logs files log_gui.txt, windscribeservice.log, and WireguardServiceLog.txt (Windows only) to one,
// cutting out the middle of the log if the count of lines exceeds MAX_COUNT_OF_LINES
class MergeLog
{
public:
    static QString mergeLogs(bool doMergePerLine);
    static QString mergePrevLogs(bool doMergePerLine);

private:
    static constexpr int MAX_COUNT_OF_LINES = 100000;
    static QString merge(const QString &guiLogFilename, const QString &serviceLogFilename, const QString &servicePrevLogFilename,
                         const QString &wireguardServiceLogFilename, const QString &installerLogFilename, bool doMergePerLine);

    enum class LineSource { GUI, SERVICE, WIREGUARD_SERVICE, NUM_LINE_SOURCES, INSTALLER };
    static int mergeTask(QMutex *mutex, QMultiMap<quint64, QPair<LineSource, QString>> *lines, const QString *filename, LineSource source, bool useMinMax, QDateTime min, QDateTime max);

    static const QString guiLogLocation();
    static const QString serviceLogLocation();
    static const QString wireguardServiceLogLocation();
    static const QString installerLogLocation();

    static const QString prevGuiLogLocation();
    static const QString prevServiceLogLocation();
    static const QString prevWireguardServiceLogLocation();
    static const QString prevInstallerLogLocation();
};
