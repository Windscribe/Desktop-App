#include "ras_service_win.h"
#include "engine/helper/helper_win.h"
#include "utils/ws_assert.h"
#include "utils/winutils.h"
#include <windows.h>
#include <QCoreApplication>
#include <QElapsedTimer>

bool RAS_Service_win::isRASRunning()
{
    return WinUtils::isServiceRunning("SstpSvc") && WinUtils::isServiceRunning("RasMan");
}

bool RAS_Service_win::restartRASServices(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    WS_ASSERT(helper_win);

    int attempts = 2;
    while (attempts > 0)
    {
        helper_win->resetAndStartRAS();

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
