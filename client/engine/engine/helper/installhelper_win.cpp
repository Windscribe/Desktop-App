#include "installhelper_win.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <windows.h>
#include <shellapi.h>

#include "utils/executable_signature/executable_signature.h"
#include "utils/logger.h"

bool InstallHelper_win::checkInstallHelper(QString &outPath)
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

QString InstallHelper_win::getPathForLogFile()
{
    // The install helper runs as root, and therefore should not log to a user writable folder.
    QString strPath = QCoreApplication::applicationDirPath();
    strPath += "/logwindscribeinstallhelper.txt";
    return strPath;
}

void InstallHelper_win::outputLogFileToLoggerAndRemove(QString &logPath)
{
    QFile file(logPath);
    if (file.open(QIODevice::ReadOnly))
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

bool InstallHelper_win::executeInstallHelperCmd(const QString &servicePath)
{
    QString utilPath;
    if (!checkInstallHelper(utilPath))
    {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe not found in the app directory";
        return false;
    }

    if (!QFile::exists(servicePath))
    {
        qCDebug(LOG_BASIC) << "Windscribe service not found in path:" << servicePath;
        return false;
    }

    QString logPath = getPathForLogFile();

    std::wstring ws = utilPath.toStdWString();
    std::wstring pars = L"/InstallService";
    pars += L" \"" + logPath.toStdWString() + L"\"";
    pars += L" \"" + servicePath.toStdWString() + L"\"";

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

    qCDebug(LOG_BASIC) << "InstallHelper::executeInstallHelperCmd ShellExecuteEx failed:" << GetLastError();
    return false;
}
