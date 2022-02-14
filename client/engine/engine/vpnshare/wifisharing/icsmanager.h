#ifndef ICSMANAGER_H
#define ICSMANAGER_H

#include <QThread>
#include <QMutex>
#include "Engine/Helper/helper_win.h"
#include <QFile>
#include <QWaitCondition>
#include <Rpc.h>

// IcsManager works through helper, because need admin rights
// and 64 bit version of helper for Win64
class IcsManager : public QThread
{
    Q_OBJECT
public:
    explicit IcsManager(QObject *parent, IHelper *helper);
    virtual ~IcsManager();

    bool isSupportedIcs();
    bool startIcs();
    bool stopIcs();
    bool changeIcsSettings(const GUID &publicGuid, const GUID &privateGuid);

    bool isUpdateIcsInProgress();

private:
    Helper_win *helper_;

    QString path_;
    QString updateIcsExePath_;
    bool bNeedFinish_;

    bool isUpdateIcsCmdInProgress_;
    unsigned long cmdIdInProgress_;
    QMutex mutexCmdInProgress_;
    HANDLE hEvent_;
    HANDLE hWaitFunc_;

    struct LAST_CMD_INFO {
        LAST_CMD_INFO() { isValid = false; }

        bool isValid;
        GUID publicGuid;
        GUID privateGuid;
    };

    LAST_CMD_INFO lastCmdInfo_;

    QString guidToStr(const GUID &guid);
    void executeNextUpdateIcsCmd(const GUID &publicGuid, const GUID &privateGuid);

    static VOID CALLBACK waitEventCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired);

};

#endif // ICSMANAGER_H
