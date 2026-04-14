#include "installhelper_win.h"

#include <QCoreApplication>
#include <QFile>

#include <windows.h>
#include <shellapi.h>

#include "utils/executable_signature/executable_signature.h"
#include "utils/log/categories.h"

namespace InstallHelper_win
{

static void outputLogFileToLoggerAndRemove(const QString &logPath)
{
    QString logFile = logPath + "/log" WS_PRODUCT_NAME_LOWER "installhelper.txt";
    QFile file(logFile);

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            qCInfo(LOG_BASIC) << in.readLine();
        }
        file.close();
        file.remove();
    }
}

bool executeInstallHelperCmd()
{
    QString installDir = QCoreApplication::applicationDirPath();

    QString installHelperExe = installDir + "/InstallHelper.exe";
    if (!QFile::exists(installHelperExe)) {
        qCCritical(LOG_BASIC) << "InstallHelper.exe not found in the app directory";
        return false;
    }

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(installHelperExe.toStdWString())) {
        qCCritical(LOG_BASIC) << "InstallHelper.exe incorrect signature:" << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    QString helperExe = installDir + "/" WS_APP_IDENTIFIER "Service.exe";
    if (!QFile::exists(helperExe)) {
        qCCritical(LOG_BASIC) << WS_PRODUCT_NAME " service not found in path:" << helperExe;
        return false;
    }

    SHELLEXECUTEINFO sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFO));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    sinfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    sinfo.hwnd = NULL;
    sinfo.lpFile = qUtf16Printable(installHelperExe);
    sinfo.lpParameters = L"/InstallService";
    sinfo.lpVerb       = L"open";
    sinfo.nShow        = SW_HIDE;

    if (ShellExecuteEx(&sinfo)) {
        WaitForSingleObject(sinfo.hProcess, INFINITE);
        CloseHandle(sinfo.hProcess);
        outputLogFileToLoggerAndRemove(installDir);
        return true;
    }

    qCCritical(LOG_BASIC) << "InstallHelper::executeInstallHelperCmd ShellExecuteEx failed:" << GetLastError();
    return false;
}

};
