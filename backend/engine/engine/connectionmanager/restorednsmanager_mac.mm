#include "restorednsmanager_mac.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include "utils/logger.h"
#include <QFile>
#include "engine/tempscripts_mac.h"

bool RestoreDNSManager_mac::restoreState(IHelper *helper)
{
    qCDebug(LOG_BASIC) << "=== RestoreDNSManager::restoreState() ===";

    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    QString strAnswer = helper->executeRootCommand(strDnsPath + " -down");
    qCDebug(LOG_BASIC) << "Output from dns.sh -down: " << strAnswer;

    strAnswer = helper->executeRootCommand(strDnsPath);
    qCDebug(LOG_BASIC) << "Output from dns.sh: " << strAnswer;
    return true;
}
