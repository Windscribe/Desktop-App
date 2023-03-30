#include "restorednsmanager_mac.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include "utils/logger.h"
#include <QFile>
#include "engine/helper/helper_mac.h"

bool RestoreDNSManager_mac::restoreState(IHelper *helper)
{
    Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper);
    qCDebug(LOG_BASIC) << "=== RestoreDNSManager::restoreState() ===";

    return helper_mac->setDnsScriptEnabled(false);
}
