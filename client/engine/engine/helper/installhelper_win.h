#pragma once

#include <QString>

class InstallHelper_win
{
public:
    static bool executeInstallHelperCmd(const QString &servicePath);

private:
    static bool checkInstallHelper(QString &outPath);
    static QString getPathForLogFile();
    static void outputLogFileToLoggerAndRemove(QString &logPath);
};
