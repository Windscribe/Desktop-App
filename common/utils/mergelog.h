#ifndef MERGELOG_H
#define MERGELOG_H

#include <QString>

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
};

#endif // MERGELOG_H
