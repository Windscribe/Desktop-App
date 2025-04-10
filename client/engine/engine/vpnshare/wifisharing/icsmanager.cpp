#include "icsmanager.h"

#include <winsock2.h>
#include <iphlpapi.h>


IcsManager::IcsManager(QObject *parent, Helper *helper) :
    QObject(parent), helper_(helper)
{
}

bool IcsManager::isSupportedIcs()
{
    return helper_->isIcsSupported();
}

bool IcsManager::startIcs()
{
    return helper_->startIcs();
}

bool IcsManager::stopIcs()
{
    return helper_->stopIcs();
}

bool IcsManager::changeIcs(const QString &adapterName)
{
    return helper_->changeIcs(adapterName);
}
