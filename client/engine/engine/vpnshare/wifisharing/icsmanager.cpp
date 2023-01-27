#include "icsmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "Utils/logger.h"

#define WINDSCRIBE_UPDATE_ICS_EVENT_NAME L"Global\\WindscribeUpdateIcsEvent1034"

IcsManager::IcsManager(QObject *parent, IHelper *helper) : QThread(parent),
    bNeedFinish_(false), isUpdateIcsCmdInProgress_(false), cmdIdInProgress_(0), hWaitFunc_(0)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_);
    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(strPath);
    dir.mkpath(strPath);
    path_ = strPath + "/ics.dat";
    hEvent_ = CreateEvent(NULL, TRUE, FALSE, WINDSCRIBE_UPDATE_ICS_EVENT_NAME);
    updateIcsExePath_ = QCoreApplication::applicationDirPath() + "/ChangeIcs.exe";
}

IcsManager::~IcsManager()
{
    QFile file(path_);
    file.remove();
    CloseHandle(hEvent_);
}

bool IcsManager::isSupportedIcs()
{
    if (!helper_->isHelperConnected())
    {
        //Q_ASSERT(false);
        return false;
    }
    return helper_->isSupportedICS();
}

bool IcsManager::startIcs()
{
    if (!helper_->isHelperConnected())
    {
        Q_ASSERT(false);
        return false;
    }

    QString cmdLine = "\"" + updateIcsExePath_ + "\" -save \"" + path_ + "\"";
    unsigned long cmdId;

    if (helper_->executeChangeIcs(0, path_, "", "", cmdId, ""))
    {
        qCDebug(LOG_WLAN_MANAGER) << cmdLine;

        QString logStr;
        bool bFinished = false;
        while (!bFinished)
        {
            helper_->getUnblockingCmdStatus(cmdId, logStr, bFinished);
            QThread::msleep(1);
        }
        qCDebug(LOG_WLAN_MANAGER) << logStr;
        return true;
    }
    else
    {
        qCDebug(LOG_WLAN_MANAGER) << "Failed start process:" << cmdLine;
        return false;
    }
}

bool IcsManager::stopIcs()
{
    if (!helper_->isHelperConnected())
    {
        //Q_ASSERT(false);
        return false;
    }

    // wait for finish update ics cmd
    while (isUpdateIcsCmdInProgress_)
    {
        QThread::msleep(1);
    }

    QString cmdLine = "\"" + updateIcsExePath_ + "\" -restore \"" + path_ + "\"";
    unsigned long cmdId;

    if (helper_->executeChangeIcs(1, path_, "", "", cmdId, ""))
    {
        qCDebug(LOG_WLAN_MANAGER) << cmdLine;

        QString logStr;
        bool bFinished = false;
        while (!bFinished)
        {
            helper_->getUnblockingCmdStatus(cmdId, logStr, bFinished);
            QThread::msleep(1);
        }

        qCDebug(LOG_WLAN_MANAGER) << logStr;
        return true;
    }
    else
    {
        qCDebug(LOG_WLAN_MANAGER) << "Failed start process:" << cmdLine;
        return false;
    }
}

bool IcsManager::changeIcsSettings(const GUID &publicGuid, const GUID &privateGuid)
{
    if (!helper_->isHelperConnected())
    {
        Q_ASSERT(false);
        return false;
    }

    // check if we already execute changeIcs command and command in progress
    QMutexLocker locker(&mutexCmdInProgress_);
    if (isUpdateIcsCmdInProgress_)
    {
        lastCmdInfo_.isValid = true;
        lastCmdInfo_.publicGuid = publicGuid;
        lastCmdInfo_.privateGuid = privateGuid;
    }
    else
    {
        executeNextUpdateIcsCmd(publicGuid, privateGuid);
    }

    return true;
}

bool IcsManager::isUpdateIcsInProgress()
{
    QMutexLocker locker(&mutexCmdInProgress_);
    return isUpdateIcsCmdInProgress_;
}

QString IcsManager::guidToStr(const GUID &guid)
{
    wchar_t szBuf[256];
    swprintf(szBuf, 256, L"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return QString::fromStdWString(szBuf);
}

void IcsManager::executeNextUpdateIcsCmd(const GUID &publicGuid, const GUID &privateGuid)
{
    ResetEvent(hEvent_);

    QString strPublicGuid = guidToStr(publicGuid);
    QString strPrivateGuid = guidToStr(privateGuid);

    QString cmdLine = "\"" + updateIcsExePath_ + "\" -change " + strPublicGuid + " " + strPrivateGuid;
    if (helper_->executeChangeIcs(2, "", strPublicGuid, strPrivateGuid, cmdIdInProgress_, QString::fromStdWString(WINDSCRIBE_UPDATE_ICS_EVENT_NAME)))
    {
        qCDebug(LOG_WLAN_MANAGER) << cmdLine;

        isUpdateIcsCmdInProgress_ = true;
        RegisterWaitForSingleObject(&hWaitFunc_, hEvent_, waitEventCallback,
                                this, INFINITE, WT_EXECUTEONLYONCE);
    }
    else
    {
        qCDebug(LOG_WLAN_MANAGER) << "Failed start process:" << cmdLine;
    }
}

void IcsManager::waitEventCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired)
{
    Q_UNUSED(timerOrWaitFired);

    IcsManager *this_ = static_cast<IcsManager *>(lpParameter);
    QMutexLocker locker(&this_->mutexCmdInProgress_);
    UnregisterWaitEx(this_->hWaitFunc_, NULL);
    QString logStr;
    bool bFinished = false;
    this_->helper_->getUnblockingCmdStatus(this_->cmdIdInProgress_, logStr, bFinished);
    qCDebug(LOG_WLAN_MANAGER) << logStr;
    this_->isUpdateIcsCmdInProgress_ = false;

    if (this_->lastCmdInfo_.isValid)
    {
        this_->executeNextUpdateIcsCmd(this_->lastCmdInfo_.publicGuid, this_->lastCmdInfo_.privateGuid);
        this_->lastCmdInfo_.isValid = false;
    }
}
