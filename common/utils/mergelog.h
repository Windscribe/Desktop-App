#ifndef MERGELOG_H
#define MERGELOG_H

#include <QDateTime>
#include <QString>
#include <QMutex>
#include <vector>

// merge two logs log_gui.txt and log_engine.txt to one
class MergeLog
{
public:
    static QString mergeLogs(bool doMergePerLine);
    static QString mergePrevLogs(bool doMergePerLine);

private:
    static QString merge(const QString &guiLogFilename, const QString &engineLogFilename,
                         const QString &serviceLogFilename, const QString &servicePrevLogFilename,
                         bool doMergePerLine);

    enum class LineSource { GUI, ENGINE, SERVICE, NUM_LINE_SOURCES };
    static int mergeTask(QMutex *mutex, QMultiMap<quint64, QPair<LineSource, QString>> *lines, const QString *filename, LineSource source, bool useMinMax, QDateTime min, QDateTime max);
};


#endif // MERGELOG_H
