#include "mergelog.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
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
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    const QString guiLogFilename = path + "/log_gui.txt";
    const QString engineLogFilename = path + "/log_engine.txt";
    const QString serviceLogFilename1 = qApp->applicationDirPath() + "/windscribeservice.log";
    const QString serviceLogFilename2 = qApp->applicationDirPath() + "/windscribeservice_prev.log";
    return merge(guiLogFilename, engineLogFilename, serviceLogFilename1, serviceLogFilename2,
                 doMergePerLine);
}

QString MergeLog::mergePrevLogs(bool doMergePerLine)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString guiLogFilename = path + "/prev_log_gui.txt";
    const QString engineLogFilename = path + "/prev_log_engine.txt";
    const QString serviceLogFilename1 = qApp->applicationDirPath() + "/windscribeservice.log";
    const QString serviceLogFilename2 = qApp->applicationDirPath() + "/windscribeservice_prev.log";
    return merge(guiLogFilename, engineLogFilename, serviceLogFilename1, serviceLogFilename2,
                 doMergePerLine);
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

QString MergeLog::merge(const QString &guiLogFilename, const QString &engineLogFilename,
                        const QString &serviceLogFilename, const QString &servicePrevLogFilename,
                        bool doMergePerLine)
{
    QMutex mutex;
    QMultiMap<quint64, QPair<LineSource, QString>> lines;
    int estimatedLogSize = 0;

    auto futureGuiLog = std::async(MergeLog::mergeTask, &mutex, &lines, &guiLogFilename, LineSource::GUI, false, QDateTime(), QDateTime());
    auto futureEngineLog = std::async(MergeLog::mergeTask, &mutex, &lines, &engineLogFilename, LineSource::ENGINE, false, QDateTime(), QDateTime());
    estimatedLogSize += futureGuiLog.get();
    estimatedLogSize += futureEngineLog.get();

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

    if (!doMergePerLine)
        estimatedLogSize += 400;  // Account for log separation lines.

    QString result;
    result.reserve(estimatedLogSize);
    auto logAppendFun = [&result](const QPair<LineSource, QString> &data) {
        switch (data.first) {
        case LineSource::GUI:
            result.append("G ");
            break;
        case LineSource::ENGINE:
            result.append("E ");
            break;
        case LineSource::SERVICE:
            result.append("S ");
            break;
        default:
            break;
        }
        result.append(data.second);
        result.append("\n");
    };

    // cut out the path of the log if the count of lines  exceeds MAX_COUNT_OF_LINES (keep 10% begin and 90% end of log)
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

