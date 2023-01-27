#include "mergelog.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QFileInfo>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <future>

namespace
{
// On Linux and Mac: big files take a while to load but can be loaded. Tested with 2GB (on linux) and 10GB file on OSX
// On Windows only:
// Application seems to crash reliably (in debug mode) at 630MB
// Selecting a slightly lower max combined file size to prevent app from crashing
// Even at 500MB, application is very slow to load (5-10s) and UI background glitches are obivous
const quint64 MAX_COMBINED_LOG_SIZE = 500000000; // 500MB

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
    else
    {
        return QDateTime();
    }
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
    else
    {
        return QDateTime();
    }
}


}  // namespace

QString MergeLog::mergeLogs(bool doMergePerLine)
{
    const QString guiLogFilename = guiLogLocation();
    const QString serviceLogFilename1 = serviceLogLocation();
    const QString serviceLogFilename2 = prevServiceLogLocation();
    const QString wgServiceLogFilename = wireguardServiceLogLocation();
    return merge(guiLogFilename, serviceLogFilename1, serviceLogFilename2,
                 wgServiceLogFilename, doMergePerLine);
}

QString MergeLog::mergePrevLogs(bool doMergePerLine)
{
    const QString guiLogFilename = prevGuiLogLocation();
    const QString serviceLogFilename1 = serviceLogLocation();
    const QString serviceLogFilename2 = prevServiceLogLocation();
    const QString wgPrevServiceLogFilename = prevWireguardServiceLogLocation();
    return merge(guiLogFilename, serviceLogFilename1, serviceLogFilename2,
                 wgPrevServiceLogFilename, doMergePerLine);
}

bool MergeLog::canMerge()
{
    quint64 mergedFileSize = 0;

    // gui
    QFileInfo guiLogInfo(guiLogLocation());
    mergedFileSize += guiLogInfo.size();

    // prev gui
    QFileInfo prevGuiLogInfo(prevGuiLogLocation());
    mergedFileSize += prevGuiLogInfo.size();

    // service (twice)
    QFileInfo serviceLogInfo(serviceLogLocation());
    mergedFileSize += serviceLogInfo.size() * 2; // why are we merging twice though, is this a bug?

    // prev service (twice)
    QFileInfo prevServiceLogInfo(prevServiceLogLocation());
    mergedFileSize += prevServiceLogInfo.size() * 2; // why are we merging twice though, is this a bug?

    QFileInfo wgServiceLog(wireguardServiceLogLocation());
    mergedFileSize += wgServiceLog.size();

    QFileInfo prevWGServiceLog(prevWireguardServiceLogLocation());
    mergedFileSize += prevWGServiceLog.size();

    return mergedFileSize < MAX_COMBINED_LOG_SIZE;
}

int MergeLog::mergeTask(QMutex *mutex, QMultiMap<quint64, QPair<LineSource, QString>> *lines, const QString *filename, LineSource source, bool useMinMax, QDateTime min, QDateTime max)
{
    int datasize = 0;
    const int kCurrentYearOffset = QDateTime::currentDateTime().date().year() - 1900;
    QDateTime prevDateTime;

    std::ifstream file;
    file.open(filename->toStdString());
    if (file.fail())
    {
        return 0;
    }

    int timestamp = 0;
    std::string line;
    while (std::getline(file, line))
    {
        if (line[0] != '[')
            continue;
        const auto datestr = line.substr(1, 19);

        const auto datetime = isYearInDatePresent(datestr)
            ? parseDateTimeFormat1(datestr)
                .addYears(100)
            : parseDateTimeFormat2(datestr)
                .addYears(kCurrentYearOffset);

        if (useMinMax && (datetime < min || datetime > max))
            continue;
        if (prevDateTime != datetime) {
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
            lines->insert(key, qMakePair(source, QString::fromStdString(line)));
        }
        datasize += line.length() + 3;
    }
    return datasize;
}

const QString MergeLog::guiLogLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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

const QString MergeLog::prevGuiLogLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/helper_no_prev_log.txt";
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

QString MergeLog::merge(const QString &guiLogFilename, const QString &serviceLogFilename,
                        const QString &servicePrevLogFilename, const QString &wireguardServiceLogFilename,
                        bool doMergePerLine)
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

