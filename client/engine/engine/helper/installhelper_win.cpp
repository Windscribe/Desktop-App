#include "installhelper_win.h"

#include <QCoreApplication>
#include <QFile>

#include <windows.h>
#include <shellapi.h>

#include "utils/executable_signature/executable_signature.h"
#include "utils/logger.h"

namespace InstallHelper_win
{

static void outputLogFileToLoggerAndRemove(const QString &logPath)
{
    QString logFile = logPath + "/logwindscribeinstallhelper.txt";
    QFile file(logFile);

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            qCDebug(LOG_BASIC) << in.readLine();
        }
        file.close();
        file.remove();
    }
}

bool executeInstallHelperCmd()
{
    QString installDir = QCoreApplication::applicationDirPath();

    QString installHelperExe = installDir + "/WindscribeInstallHelper.exe";
    if (!QFile::exists(installHelperExe)) {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe not found in the app directory";
        return false;
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(installHelperExe.toStdWString())) {
        qCDebug(LOG_BASIC) << "WindscribeInstallHelper.exe incorrect signature:" << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    QString helperExe = installDir + "/WindscribeService.exe";
    if (!QFile::exists(helperExe)) {
        qCDebug(LOG_BASIC) << "Windscribe service not found in path:" << helperExe;
        return false;
    }

    SHELLEXECUTEINFO sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    sinfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    sinfo.hwnd = NULL;
    sinfo.lpFile = installHelperExe.toStdWString().c_str();
    sinfo.lpParameters = L"/InstallService";
    sinfo.lpVerb       = L"runas";
    sinfo.nShow        = SW_HIDE;

    if (ShellExecuteEx(&sinfo)) {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
        outputLogFileToLoggerAndRemove(installDir);
        return true;
    }

    qCDebug(LOG_BASIC) << "InstallHelper::executeInstallHelperCmd ShellExecuteEx failed:" << GetLastError();
    return false;
}

};
