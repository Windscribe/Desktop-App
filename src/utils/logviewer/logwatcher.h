#pragma once

#include <memory>
#include <QMap>
#include <QMutex>
#include <QThread>
#include "common.h"


class LogWatcher final : public QThread
{
    Q_OBJECT

public:
    LogWatcher();
    ~LogWatcher();
    LogWatcher(const LogWatcher&) = delete;
    LogWatcher &operator=(const LogWatcher&) = delete;

    bool add(const QString &filename, LogDataType type, LogRangeCheckType rangeCheck);
    void remove(LogDataType type);
    void removeAll();

    static LogDataType detectLogType(const QString &filename);

signals:
    void logLinesReady(QStringList lines, LogDataType type, quint32 index,
                       LogRangeCheckType rangeCheck);
    void logTypeRemoved(LogDataType type);
    void logIndexRemoved(quint32 index);

private:
    struct LogFileInfo {
        LogFileInfo() : position(0), datasize(-1), index(0), type(LOG_TYPE_UNKNOWN),
                        rangecheck(LogRangeCheckType::NONE) {}
        LogFileInfo(LogDataType theType, quint32 theIndex, LogRangeCheckType rangeCheck)
            : position(0), datasize(-1), index(theIndex), type(theType), rangecheck(rangeCheck) {}
        qint64 position;
        qint64 datasize;
        quint32 index;
        LogDataType type;
        LogRangeCheckType rangecheck;
    };
    using LogFileMutableIterator = QMutableMapIterator<QString, LogFileInfo>;
    using LogFileStorage = QMultiMap<QString, LogFileInfo>;

    void run() override;
    void process(const QString &filename, LogFileInfo *info);

    QMutex mutex_;
    LogFileStorage logs_;
    quint32 log_index_;
    bool is_watch_done_;
};
