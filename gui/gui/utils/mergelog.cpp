#include "mergelog.h"
#include <QStandardPaths>
#include <QFile>

QString MergeLog::mergeLogs()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    return merge(path + "/log_gui.txt", path + "/log_engine.txt");
}

QString MergeLog::mergePrevLogs()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    return merge(path + "/prev_log_gui.txt", path + "/prev_log_engine.txt");
}

QString MergeLog::merge(const QString &path1, const QString &path2)
{
    QString ret;
    QFile file1(path1);
    QFile file2(path2);

    if (file1.open(QIODevice::ReadOnly))
    {
        if (file2.open(QIODevice::ReadOnly))
        {
            // both files opened
            ret += file1.readAll();
            ret += "---Engine--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
            ret += file2.readAll();
        }
        else
        {
            //only file1 opened
            ret += file1.readAll();
        }
    }
    else if (file2.open(QIODevice::ReadOnly))
    {
        //only file2 opened
        ret += "---Engine--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        ret += file2.readAll();
    }

    return ret;
}
