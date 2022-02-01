#include "restorednsmanager_mac.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include "utils/logger.h"
#include <QFile>
#include "engine/tempscripts_mac.h"
#include "engine/helper/helper_mac.h"

bool RestoreDNSManager_mac::restoreState(IHelper *helper)
{
    Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper);
    qCDebug(LOG_BASIC) << "=== RestoreDNSManager::restoreState() ===";

    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    if (strDnsPath.isEmpty()) {
        return false;
    }
    QString strAnswer = helper_mac->executeRootCommand(strDnsPath + " -down");
    qCDebug(LOG_BASIC) << "Output from dns.sh -down: " << strAnswer;

    strAnswer = helper_mac->executeRootCommand(strDnsPath);
    qCDebug(LOG_BASIC) << "Output from dns.sh: " << strAnswer;
    return true;
}
