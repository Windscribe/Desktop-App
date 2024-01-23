#include "mergelog.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include <future>

namespace
{
bool isYearInDatePresent(const std::string &dateline)
{
    const int scan = qMin(6, (int)dateline.size());
    for (int i = 0; i < scan; ++i)
        if (dateline[i] == ' ')
            return false;
    return true;
}

// parse date from string "ddMMyy hh:mm:ss:zzz"
QDateTime parseDateTimeFormat1(const std::string &datestr)
{
    int dd,MM,yy;
    int hh, mm, ss, zzz;
    if (sscanf(datestr.c_str(),"%02d%02d%02d %02d:%02d:%02d:%03d", &dd, &MM, &yy, &hh, &mm, &ss, &zzz) == 7)
    {
        return QDateTime(QDate(yy + 1900, MM, dd), QTime(hh, mm, ss, zzz));
    }

    return QDateTime();
}

// parse date from string "ddMM hh:mm:ss:zzz"
QDateTime parseDateTimeFormat2(const std::string &datestr)
{
    int dd,MM;
    int hh, mm, ss, zzz;
    if (sscanf(datestr.c_str(),"%02d%02d %02d:%02d:%02d:%03d", &dd, &MM, &hh, &mm, &ss, &zzz) == 6)
    {
        return QDateTime(QDate(1900, MM, dd), QTime(hh, mm, ss, zzz));
    }

    return QDateTime();
}

}  // namespace

QString MergeLog::mergeLogs(bool doMergePerLine)
{
    const QString guiLogFilename = guiLogLocation();
    const QString serviceLogFilename1 = serviceLogLocation();
    const QString serviceLogFilename2 = prevServiceLogLocation();
    const QString wgServiceLogFilename = wireguardServiceLogLocation();
    const QString installerLogFilename = installerLogLocation();
    return merge(guiLogFilename, serviceLogFilename1, serviceLogFilename2,
                 wgServiceLogFilename, installerLogFilename, doMergePerLine);
}

QString MergeLog::mergePrevLogs(bool doMergePerLine)
{
    const QString guiLogFilename = prevGuiLogLocation();
    const QString serviceLogFilename1 = serviceLogLocation();
    const QString serviceLogFilename2 = prevServiceLogLocation();
    const QString wgPrevServiceLogFilename = prevWireguardServiceLogLocation();
    const QString installerPrevLogFilename = prevInstallerLogLocation();
    return merge(guiLogFilename, serviceLogFilename1, serviceLogFilename2,
                 wgPrevServiceLogFilename, installerPrevLogFilename, doMergePerLine);
}

int MergeLog::mergeTask(QMutex *mutex, QMultiMap<quint64, QPair<LineSource, QString>> *lines, const QString *filename, LineSource source, bool useMinMax, QDateTime min, QDateTime max)
{
    int datasize = 0;
    const int kCurrentYearOffset = QDateTime::currentDateTime().date().year() - 1900;
    QDateTime prevDateTime;

    QFile file(*filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }

    int timestamp = 0;
    QTextStream textStream(&file);
    // If file is larger than 10MiB, just take the last 10MiB
    int64_t filelen = file.size();
    if (filelen > 10000000) {
        file.seek(filelen - 10000000);
    }

    while (!textStream.atEnd())
    {
        QString line = textStream.readLine();

        if (line[0] != '[' || line.length() < 20)
            continue;

        const auto datestr = line.sliced(1, 19).toStdString();
        const auto datetime = isYearInDatePresent(datestr)
                                  ? parseDateTimeFormat1(datestr)
                                        .addYears(100)
                                  : parseDateTimeFormat2(datestr)
                                        .addYears(kCurrentYearOffset);

        if (useMinMax && (datetime < min || datetime > max))
            continue;

        // Installer on Mac can have inconsistency in times because of native mac api.
        // In the example below we have the same timestamp but different time since start.
        // [110124 20:16:15:449      0.002] CPU architecture: arm64)
        // [110124 20:16:15:449      0.003] MacOS version: Version 14.1 (Build 23B74)
        // It is necessary to calculate time there to increase timestamp var by 1 for each line.
        // It is acceptable as times of installer do not intersect other times.
        if (prevDateTime != datetime && source != LineSource::INSTALLER) {
            prevDateTime = datetime;
            timestamp = 0;
        }

        // In QMultiMap, elements with the same key will be placed in a reverse order.
        // https://doc.qt.io/qt-5/qmap-iterator.html#details
        // To deal with the issue, we create a compound key: 44 bits for a timestamp, 2 bits
        // for the source, and 18 additional lower bits for a "timestamp" (line serial
        // number). We assume the maximum number of lines with unique timestamp never
        // exceeds 262143, which is quite reasonable (it looks infeasible to output 250k+
        // lines to a log file within 1 ms).
        const auto key = (static_cast<quint64>(datetime.toMSecsSinceEpoch()) << 20)
                       | (static_cast<quint64>(source) << 18) | qMin(timestamp++, 0x3ffff);

        {
            QMutexLocker locker(mutex);
            lines->insert(key, qMakePair(source, line));
        }
        datasize += line.length() + 3;
    }
    return datasize;
}

const QString MergeLog::guiLogLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return path + "/log_gui.txt";
}

const QString MergeLog::serviceLogLocation()
{
#if defined(Q_OS_LINUX)
    return qApp->applicationDirPath() + "/helper_log.txt";
#elif defined(Q_OS_MACOS)
    return "/Library/Logs/com.windscribe.helper.macos/helper_log.txt";
#else
    return qApp->applicationDirPath() + "/windscribeservice.log";
#endif
}

const QString MergeLog::wireguardServiceLogLocation()
{
#if defined(Q_OS_LINUX)
    return qApp->applicationDirPath() + "/no_wg_service_log_linux.txt";
#elif defined(Q_OS_MACOS)
    return qApp->applicationDirPath() + "/no_wg_service_log_macos.txt";
#else
    return qApp->applicationDirPath() + "/WireguardServiceLog.txt";
#endif
}

const QString MergeLog::installerLogLocation()
{
#if defined(Q_OS_LINUX)
    return "";
#elif defined(Q_OS_MACOS)
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/../Windscribe/log_installer.txt";
#else
    return qApp->applicationDirPath() + "/log_installer.txt";
#endif
}

const QString MergeLog::prevGuiLogLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return path + "/prev_log_gui.txt";
}

const QString MergeLog::prevServiceLogLocation()
{
#if defined(Q_OS_LINUX)
    // The Linux helper does not currently maintain a previous log file, as it doesn't log
    // very much information, except during a failure condition.
    return qApp->applicationDirPath() + "/helper_no_prev_log.txt";
#elif defined(Q_OS_MACOS)
    // The Mac helper does not currently maintain a previous log file
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/helper_no_prev_log.txt";
#else
    return qApp->applicationDirPath() + "/windscribeservice_prev.log";
#endif
}

const QString MergeLog::prevWireguardServiceLogLocation()
{
#if defined(Q_OS_LINUX)
    return qApp->applicationDirPath() + "/no_wg_service_log_linux.txt";
#elif defined(Q_OS_MACOS)
    return qApp->applicationDirPath() + "/no_wg_service_log_macos.txt";
#else
    return qApp->applicationDirPath() + "/WireguardServiceLog_prev.txt";
#endif
}

const QString MergeLog::prevInstallerLogLocation()
{
#if defined(Q_OS_LINUX)
    return "";
#elif defined(Q_OS_MACOS)
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/../Windscribe/prev_log_installer.txt";
#else
    return qApp->applicationDirPath() + "/prev_log_installer.txt";
#endif
}

QString MergeLog::merge(const QString &guiLogFilename, const QString &serviceLogFilename,
                        const QString &servicePrevLogFilename, const QString &wireguardServiceLogFilename,
                        const QString &installerLogFilename, bool doMergePerLine)
{
    QMutex mutex;
    QMultiMap<quint64, QPair<LineSource, QString>> lines;
    int estimatedLogSize = 0;

    auto futureGuiLog = std::async(MergeLog::mergeTask, &mutex, &lines, &guiLogFilename, LineSource::GUI, false, QDateTime(), QDateTime());
    estimatedLogSize += futureGuiLog.get();

    QDateTime minDate, maxDate;
    bool isUseMinMaxDate = false;
    if (lines.count() > 1)
    {
        minDate = QDateTime::fromMSecsSinceEpoch(lines.firstKey() >> 20); // Strip source and timestamp.
        maxDate = QDateTime::fromMSecsSinceEpoch(lines.lastKey() >> 20);
        isUseMinMaxDate = true;
    }

    auto futureService = std::async(MergeLog::mergeTask, &mutex, &lines, &serviceLogFilename, LineSource::SERVICE, isUseMinMaxDate, minDate, maxDate);
    auto futureServicePrev = std::async(MergeLog::mergeTask, &mutex, &lines, &servicePrevLogFilename, LineSource::SERVICE, isUseMinMaxDate, minDate, maxDate);
    estimatedLogSize += futureService.get() + futureServicePrev.get();

    auto futureWGService = std::async(MergeLog::mergeTask, &mutex, &lines, &wireguardServiceLogFilename, LineSource::WIREGUARD_SERVICE, isUseMinMaxDate, minDate, maxDate);
    estimatedLogSize += futureWGService.get();

    if (!installerLogFilename.isEmpty()) {
        const auto installerMinDate = minDate.addDays(-7);
        auto futureInstaller = std::async(MergeLog::mergeTask, &mutex, &lines, &installerLogFilename, LineSource::INSTALLER, isUseMinMaxDate, installerMinDate, maxDate);
        estimatedLogSize += futureInstaller.get();
    }

    if (!doMergePerLine)
        estimatedLogSize += 400;  // Account for log separation lines.

    QString result;
    result.reserve(estimatedLogSize);
    auto logAppendFun = [&result](const QPair<LineSource, QString> &data) {
        switch (data.first) {
        case LineSource::GUI:
            result.append("G ");
            break;
        case LineSource::SERVICE:
            result.append("S ");
            break;
        case LineSource::WIREGUARD_SERVICE:
            result.append("W ");
            break;
        case LineSource::INSTALLER:
            result.append("I ");
            break;
        default:
            break;
        }

        result.append(data.second);
        result.append("\n");
    };

    // cut out the part of the log if the count of lines  exceeds MAX_COUNT_OF_LINES (keep 10% begin and 90% end of log)
    int cutCount = 0;
    int cutBeginInd = 0;
    int cutEndInd = lines.count();
    if (lines.count() > MAX_COUNT_OF_LINES)
    {
        cutCount = lines.count() - MAX_COUNT_OF_LINES;
        cutBeginInd = MAX_COUNT_OF_LINES / 10;
        cutEndInd = lines.count() - MAX_COUNT_OF_LINES * 0.9;
    }

    if (doMergePerLine) {
        int ind = 0;
        for (auto it = lines.constBegin(); it != lines.constEnd(); ++it)
        {
            // cut out middle
            if (cutCount == 0 || ind < cutBeginInd || ind > cutEndInd)
            {
                logAppendFun(it.value());
            }
            ind++;
        }
    } else {
        const char *separators[] = { nullptr, "Engine", "Service" };
        for (int i = 0; i < static_cast<int>(LineSource::NUM_LINE_SOURCES); ++i) {
            const auto current_source = static_cast<LineSource>(i);
            bool is_first_line = true;
            int ind = 0;
            for (auto it = lines.constBegin(); it != lines.constEnd(); ++it) {
                if (it.value().first != current_source)
                {
                    ind++;
                    continue;
                }
                if (is_first_line) {
                    is_first_line = false;
                    if (separators[i]) {
                        result.append("---");
                        result.append(separators[i]);
                        result.append(QString().fill('-',189));
                        result.append("\n");
                    }
                }
                // cut out middle
                if (cutCount == 0 || ind < cutBeginInd || ind > cutEndInd)
                {
                    logAppendFun(it.value());
                }
                ind++;
            }
        }
    }
    return result;
}

