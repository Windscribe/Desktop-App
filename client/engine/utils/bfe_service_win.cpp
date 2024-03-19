#include "bfe_service_win.h"

#include <QCoreApplication>
#include <QElapsedTimer>

#include "engine/helper/helper_win.h"
#include "utils/logger.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

bool BFE_Service_win::isBFEEnabled()
{
    return WinUtils::isServiceRunning("BFE");
}

void BFE_Service_win::enableBFE(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    WS_ASSERT(helper_win);
    QString strLog = helper_win->enableBFE();
    qCDebug(LOG_BASIC) << "Enable BFE; Answer:" << strLog;
}

bool BFE_Service_win::checkAndEnableBFE(IHelper *helper)
{
    if (isBFEEnabled()) {
        qCDebug(LOG_BASIC) << "Base filtering platform service is running";
        return true;
    }

    qCDebug(LOG_BASIC) << "Base filtering platform service is not running";

    int attempts = 2;
    while (attempts > 0) {
        enableBFE(helper);
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        while (elapsedTimer.elapsed() < 3000) {
            QCoreApplication::processEvents();
            Sleep(1);

            if (isBFEEnabled()) {
                qCDebug(LOG_BASIC) << "Base filtering platform service is now running";
                return true;
            }
        }
        attempts--;
    }

    return false;
}

BFE_Service_win::BFE_Service_win()
{
}
