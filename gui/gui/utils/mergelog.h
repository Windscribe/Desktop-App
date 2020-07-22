#ifndef MERGELOG_H
#define MERGELOG_H

#include <QString>

// merge two logs log_gui.txt and log_engine.txt to one
class MergeLog
{
public:
    static QString mergeLogs();
    static QString mergePrevLogs();

private:
    static QString merge(const QString &path1, const QString &path2);
};

#endif // MERGELOG_H
