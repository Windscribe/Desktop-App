#include "mergelog.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

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

QString MergeLog::merge(const QString &guiLogFilename, const QString &engineLogFilename,
                        const QString &serviceLogFilename, const QString &servicePrevLogFilename,
                        bool doMergePerLine)
{
    enum class LineSource { GUI, ENGINE, SERVICE, NUM_LINE_SOURCES };
    QMultiMap<quint64, QPair<LineSource, QString>> lines;
    int estimatedLogSize = 0;

    auto processFileFun = [&lines]
    (const QString &file, LineSource source, const QDateTime *minmax) -> int
    {
        QFile qf(file);
        int datasize = 0;
        const int kCurrentYearOffset = QDateTime::currentDateTime().date().year() - 1900;
        QDateTime prevDateTime;
        if (qf.open(QIODevice::ReadOnly)) {
            int timestamp = 0;
            QTextStream qts(&qf);
            while (!qts.atEnd()) {
                const QString line = qts.readLine();
                if (line[0] != '[')
                    continue;
                const auto datetime =
                    QDateTime::fromString(line.mid(1, 17), "ddMM hh:mm:ss:zzz")
                    .addYears(kCurrentYearOffset);
                if (minmax && (datetime < minmax[0] || datetime > minmax[1]))
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
                lines.insert(key, qMakePair(source, line));
                datasize += line.length() + 3;
            }
            qf.close();
        }
        return datasize;
    };

    estimatedLogSize += processFileFun(guiLogFilename, LineSource::GUI, nullptr);
    estimatedLogSize += processFileFun(engineLogFilename, LineSource::ENGINE, nullptr);

    const QDateTime minMaxTime[] = {
        QDateTime::fromMSecsSinceEpoch(lines.firstKey() >> 20),  // Strip source and timestamp.
        QDateTime::fromMSecsSinceEpoch(lines.lastKey() >> 20)
    };
    estimatedLogSize += processFileFun(serviceLogFilename, LineSource::SERVICE, minMaxTime);
    estimatedLogSize += processFileFun(servicePrevLogFilename, LineSource::SERVICE, minMaxTime);

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

    if (doMergePerLine) {
        for (auto it = lines.constBegin(); it != lines.constEnd(); ++it)
            logAppendFun(it.value());
    } else {
        const char *separators[] = { nullptr, "Engine", "Service" };
        for (int i = 0; i < static_cast<int>(LineSource::NUM_LINE_SOURCES); ++i) {
            const auto current_source = static_cast<LineSource>(i);
            bool is_first_line = true;
            for (auto it = lines.constBegin(); it != lines.constEnd(); ++it) {
                if (it.value().first != current_source)
                    continue;
                if (is_first_line) {
                    is_first_line = false;
                    if (separators[i]) {
                        result.append("---");
                        result.append(separators[i]);
                        result.append(QString().fill('-',189));
                        result.append("\n");
                    }
                }
                logAppendFun(it.value());
            }
        }
    }
    return result;
}
