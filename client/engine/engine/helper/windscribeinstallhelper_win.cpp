#include "windscribeinstallhelper_win.h"
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include "utils/logger.h"
#include <windows.h>
#include <shellapi.h>
#include "utils/executable_signature/executable_signature.h"
#include "utils/winutils.h"

bool WindscribeInstallHelper_win::checkWindscribeInstallHelper(QString &outPath)
{
    QString strPath = QCoreApplication::applicationDirPath();
    strPath += "/WindscribeInstallHelper.exe";

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(strPath.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe incorrect signature: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    outPath = strPath;
    return QFile::exists(strPath);
}

QString WindscribeInstallHelper_win::getPathForLogFile()
{
    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(strPath);
    dir.mkpath(strPath);
    return strPath + "/logwindscribeinstallhelper.txt";
}

void WindscribeInstallHelper_win::outputLogFileToLoggerAndRemove(QString &logPath)
{
    QFile file(logPath);
    if (file.open(QIODeviceBase::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            qCDebug(LOG_BASIC) << line;
        }
        file.close();
        file.remove();
    }
}

bool WindscribeInstallHelper_win::executeInstallHelperCmd(const QString &servicePath, const QString &subinaclPath)
{
    QString utilPath;
    if (!checkWindscribeInstallHelper(utilPath))
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe not found in the app directory";
        return false;
    }

    if (!QFile::exists(servicePath))
    {
        qCDebug(LOG_BASIC) << "WindscribeService not found in path:" << servicePath;
        return false;
    }
    if (!QFile::exists(subinaclPath))
    {
        qCDebug(LOG_BASIC) << "subinacl util not found in path:" << subinaclPath;
        return false;
    }

    QString logPath = getPathForLogFile();

    std::wstring ws = utilPath.toStdWString();
    std::wstring pars = L"/InstallService";
    pars += L" \"" + logPath.toStdWString() + L"\"";
    pars += L" \"" + servicePath.toStdWString() + L"\"";
    pars += L" \"" + subinaclPath.toStdWString() + L"\"";

    SHELLEXECUTEINFO sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    sinfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    sinfo.hwnd = NULL;
    sinfo.lpFile = ws.c_str();
    sinfo.lpParameters = pars.c_str();
    sinfo.lpVerb       = L"runas";
    sinfo.nShow        = SW_HIDE;

    if (ShellExecuteEx(&sinfo))
    {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
        outputLogFileToLoggerAndRemove(logPath);
        return true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper::executeInstallHelperCmd, ShellExecuteEx failed:" << GetLastError();
        return false;
    }
}

bool WindscribeInstallHelper_win::executeInstallTapCmd(const QString &tapInstallPath, const QString &infPath)
{
    QString utilPath;
    if (!checkWindscribeInstallHelper(utilPath))
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe not found in the app directory";
        return false;
    }

    QString logPath = getPathForLogFile();

    std::wstring ws = utilPath.toStdWString();
    std::wstring pars = L"/InstallTap";
    pars += L" \"" + logPath.toStdWString() + L"\"";
    pars += L" \"" + tapInstallPath.toStdWString() + L"\"";
    pars += L" \"" + infPath.toStdWString() + L"\"";

    SHELLEXECUTEINFO sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    sinfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    sinfo.hwnd = NULL;
    sinfo.lpFile = ws.c_str();
    sinfo.lpParameters = pars.c_str();
    sinfo.lpVerb       = L"runas";
    sinfo.nShow        = SW_HIDE;

    if (ShellExecuteEx(&sinfo))
    {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
        outputLogFileToLoggerAndRemove(logPath);
        return true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper::executeInstallTapCmd, ShellExecuteEx failed:" << GetLastError();
        return false;
    }

    return true;
}

bool WindscribeInstallHelper_win::executeRemoveAndInstallTapCmd(const QString &tapInstallPath, const QString &tapUninstallPath, const QString &infPath)
{
    QString utilPath;
    if (!checkWindscribeInstallHelper(utilPath))
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe not found in the app directory";
        return false;
    }

    QString logPath = getPathForLogFile();

    std::wstring ws = utilPath.toStdWString();
    std::wstring pars = L"/InstallTapWithRemovePrevious";
    pars += L" \"" + logPath.toStdWString() + L"\"";
    pars += L" \"" + tapInstallPath.toStdWString() + L"\"";
    pars += L" \"" + infPath.toStdWString() + L"\"";
    pars += L" \"" + tapUninstallPath.toStdWString() + L"\"";

    SHELLEXECUTEINFO sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    sinfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    sinfo.hwnd = NULL;
    sinfo.lpFile = ws.c_str();
    sinfo.lpParameters = pars.c_str();
    sinfo.lpVerb       = L"runas";
    sinfo.nShow        = SW_HIDE;

    if (ShellExecuteEx(&sinfo))
    {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
        outputLogFileToLoggerAndRemove(logPath);
        return true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "executeRemoveAndInstallTapCmd::executeInstallTapCmd, ShellExecuteEx failed:" << GetLastError();
        return false;
    }

    return true;
}
