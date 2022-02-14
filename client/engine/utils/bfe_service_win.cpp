#include "bfe_service_win.h"
#include "engine/helper/helper_win.h"
#include "utils/winutils.h"
#include <windows.h>
#include "utils/logger.h"
#include <QCoreApplication>
#include <QElapsedTimer>

bool BFE_Service_win::isBFEEnabled()
{
    return WinUtils::isServiceRunning("BFE");
}

void BFE_Service_win::enableBFE(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_win);
    QString strLog = helper_win->enableBFE();
    qCDebug(LOG_BASIC) << "Enable BFE; Answer:" << strLog;
}

bool BFE_Service_win::checkAndEnableBFE(IHelper *helper)
{
    bool bBFEisRunning = isBFEEnabled();
    qCDebug(LOG_BASIC) << "Base filtering platform service is running:" << bBFEisRunning;
    if (!bBFEisRunning)
    {
        int attempts = 2;
        while (attempts > 0)
        {
            enableBFE(helper);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();

            while (elapsedTimer.elapsed() < 3000)
            {
                QCoreApplication::processEvents();
                Sleep(1);

                if (isBFEEnabled())
                {
                    return true;
                }
            }
            attempts--;
        }
    }

    return false;
}

BFE_Service_win::BFE_Service_win()
{

}
