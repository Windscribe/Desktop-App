#include "logwatcher.h"

#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QTextStream>


LogWatcher::LogWatcher() : log_index_(0), is_watch_done_(false)
{
    start(NormalPriority);
}

LogWatcher::~LogWatcher()
{
    is_watch_done_ = true;
    wait();
}

bool LogWatcher::add(const QString &filename, LogDataType type, LogRangeCheckType rangeCheck)
{
    for (QFile qf(filename);;) {
        if (!qf.open(QIODeviceBase::ReadOnly))
            return false;
        qf.close();
        break;
    }
    QMutexLocker lock(&mutex_);
    auto it = logs_.find(filename);
    if (it == logs_.end()) {
        it = logs_.insert(filename, LogFileInfo(type, ++log_index_, rangeCheck));
    } else {
        emit logIndexRemoved(it->index);
        it->datasize = 0;
        it->position = 0;
        it->rangecheck = rangeCheck;
    }
    return true;
}

void LogWatcher::remove(LogDataType type)
{
    bool removed = false;
    for (QMutexLocker lock(&mutex_);;) {
        LogFileMutableIterator it(logs_);
        while (it.hasNext()) {
            it.next();
            if (type == LOG_TYPE_MIXED || it.value().type == type) {
                it.remove();
                removed = true;
            }
        }
        break;
    }
    if (removed)
        emit logTypeRemoved(type);
}

void LogWatcher::removeAll()
{
    this->remove(LOG_TYPE_MIXED);
}

void LogWatcher::run()
{
    QStringList currentLogFiles;
    QList<LogFileInfo> currentLogInfo;

    while (!is_watch_done_) {
        // Pick a modified log to process.
        mutex_.lock();
        for (auto it = logs_.begin(); it != logs_.end(); ++it) {
            const auto datasize = QFileInfo(it.key()).size();
            if (datasize != it->datasize) {
                currentLogFiles.append(it.key());
                currentLogInfo.append(it.value());
            }
        }
        mutex_.unlock();
        if (!currentLogFiles.isEmpty()) {
            for (int i = 0; i < currentLogFiles.size(); ++i)
                process(currentLogFiles[i], &currentLogInfo[i]);
            // Update log info.
            mutex_.lock();
            for (int i = 0; i < currentLogFiles.size(); ++i) {
                auto it = logs_.find(currentLogFiles[i]);
                if (it != logs_.end()) {
                    it->position = currentLogInfo[i].position;
                    it->datasize = currentLogInfo[i].datasize;
                }
            }
            mutex_.unlock();
            currentLogFiles.clear();
            currentLogInfo.clear();
        } else {
            // Nothing updated: yield resources for 50ms.
            QThread::msleep(50);
        }
    }
}

void LogWatcher::process(const QString &filename, LogFileInfo *info)
{
    Q_ASSERT(!filename.isEmpty() && info);
    auto rangeCheck = info->rangecheck;
    QFileInfo fi(filename);
    const auto current_datasize = fi.size();
    if (info->datasize > current_datasize) {
        info->position = 0;
        emit logIndexRemoved(info->index);
    } else {
        if (rangeCheck == LogRangeCheckType::MIN_TO_MAX)
            rangeCheck = LogRangeCheckType::MIN_TO_NOW;
    }
    info->datasize = current_datasize;
    if (!current_datasize)
        return;
    QStringList lines;
    QFile qf(filename);
    if (!qf.open(QIODeviceBase::ReadOnly))
        return;
    QTextStream qts(&qf);
    qts.seek(info->position);
    while (!qts.atEnd())
        lines.append(qts.readLine());
    info->position = qts.pos();
    qf.close();
    if (!lines.empty())
        emit logLinesReady(lines, info->type, info->index, rangeCheck);
}

// static
LogDataType LogWatcher::detectLogType(const QString &filename)
{
    QFile qf(filename);
    if (qf.open(QIODeviceBase::ReadOnly)) {
        QTextStream qts(&qf);
        QString line;
        while (line.isEmpty() || line.startsWith("==="))
            line = qts.readLine();
        qf.close();
        if (line.startsWith("G [") || line.startsWith("E [") || line.startsWith("S ["))
            return LOG_TYPE_MIXED;
    }
    if (filename.contains("engine"))
        return LOG_TYPE_ENGINE;
    if (filename.contains("gui"))
        return LOG_TYPE_GUI;
    if (filename.contains("service"))
        return LOG_TYPE_SERVICE;
    return LOG_TYPE_UNKNOWN;
}
