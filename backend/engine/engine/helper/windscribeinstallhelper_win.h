#ifndef WINDSCRIBEINSTALLHELPER_WIN_H
#define WINDSCRIBEINSTALLHELPER_WIN_H

#include <QString>

class WindscribeInstallHelper_win
{
public:
    static bool executeInstallHelperCmd(const QString &servicePath, const QString &subinaclPath);
    static bool executeInstallTapCmd(const QString &tapInstallPath, const QString &infPath);
    static bool executeRemoveAndInstallTapCmd(const QString &tapInstallPath, const QString &tapUninstallPath, const QString &infPath);

private:
    static bool checkWindscribeInstallHelper(QString &outPath);
    static QString getPathForLogFile();
    static void outputLogFileToLoggerAndRemove(QString &logPath);
};

#endif // WINDSCRIBEINSTALLHELPER_WIN_H
