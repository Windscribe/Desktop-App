#include "bfe_service_win.h"

#include <QCoreApplication>
#include <QElapsedTimer>

#include "utils/log/categories.h"
#include "utils/winutils.h"

bool BFE_Service_win::isBFEEnabled()
{
    return WinUtils::isServiceRunning("BFE");
}

void BFE_Service_win::enableBFE(Helper *helper)
{
    QString strLog = helper->enableBFE();
    qCInfo(LOG_BASIC) << "Enable BFE; Answer:" << strLog;
}

bool BFE_Service_win::checkAndEnableBFE(Helper *helper)
{
    if (isBFEEnabled()) {
        qCInfo(LOG_BASIC) << "Base filtering platform service is running";
        return true;
    }

    qCInfo(LOG_BASIC) << "Base filtering platform service is not running";

    int attempts = 2;
    while (attempts > 0) {
        enableBFE(helper);
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        while (elapsedTimer.elapsed() < 3000) {
            QCoreApplication::processEvents();
            Sleep(1);

            if (isBFEEnabled()) {
                qCInfo(LOG_BASIC) << "Base filtering platform service is now running";
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
