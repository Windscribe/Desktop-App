#include "restorednsmanager_mac.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include "utils/log/categories.h"
#include <QFile>

void RestoreDNSManager_mac::restoreState(Helper *helper)
{
    qCDebug(LOG_BASIC) << "RestoreDNSManager::restoreState()";
    helper->setDnsScriptEnabled(false);
}
