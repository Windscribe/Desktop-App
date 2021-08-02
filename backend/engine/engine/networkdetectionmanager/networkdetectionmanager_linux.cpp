#include "networkdetectionmanager_linux.h"

#include <QRegularExpression>
#include "../networkstatemanager/reachabilityevents.h"
#include "utils/macutils.h"
#include "utils/utils.h"
#include "utils/logger.h"

const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

NetworkDetectionManager_linux::NetworkDetectionManager_linux(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
{
    Q_UNUSED(helper);
}

NetworkDetectionManager_linux::~NetworkDetectionManager_linux()
{
}

void NetworkDetectionManager_linux::updateCurrentNetworkInterface(bool requested)
{
    Q_UNUSED(requested);
}

bool NetworkDetectionManager_linux::isOnline()
{
    return true;
}



