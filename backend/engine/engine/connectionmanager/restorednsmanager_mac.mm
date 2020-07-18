#include "restorednsmanager_mac.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include "utils/logger.h"
#include <QFile>
#include "engine/tempscripts_mac.h"

RestoreDNSManager_mac::RestoreDNSManager_mac(IHelper *helper) :
    helper_(helper)
{
}

bool RestoreDNSManager_mac::restoreState()
{
    qCDebug(LOG_BASIC) << "=== RestoreDNSManager::restoreState() ===";

    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    QString strAnswer = helper_->executeRootCommand(strDnsPath + " -down");
    qCDebug(LOG_BASIC) << "Output from dns.sh -down: " << strAnswer;

    strAnswer = helper_->executeRootCommand(strDnsPath);
    qCDebug(LOG_BASIC) << "Output from dns.sh: " << strAnswer;
    return true;
}
