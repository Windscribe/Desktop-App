#include "ras_service_win.h"
#include "engine/helper/ihelper.h"
#include "utils/winutils.h"
#include <windows.h>
#include "utils/logger.h"
#include <QCoreApplication>
#include <QElapsedTimer>

bool RAS_Service_win::isRASRunning()
{
    return WinUtils::isServiceRunning("SstpSvc") && WinUtils::isServiceRunning("RasMan");
}

bool RAS_Service_win::restartRASServices(IHelper *helper)
{
    int attempts = 2;
    while (attempts > 0)
    {
        helper->resetAndStartRAS();

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();
        while (elapsedTimer.elapsed() < 3000)
        {
            QCoreApplication::processEvents();
            Sleep(1);

            if (isRASRunning())
            {
                return true;
            }
        }
        attempts--;
    }

    return false;
}

RAS_Service_win::RAS_Service_win()
{

}
