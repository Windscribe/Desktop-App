#include "icsmanager.h"
#include "utils/ws_assert.h"

#include <winsock2.h>
#include <iphlpapi.h>


IcsManager::IcsManager(QObject *parent, IHelper *helper) : QObject(parent)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    WS_ASSERT(helper_);
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
