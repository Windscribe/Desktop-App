#include "connectionadapterinfo.h"
#include "utils/logger.h"

const QString ConnectionAdapterInfo::wintunAdapterName = "Windscribe Windtun420";
const QString ConnectionAdapterInfo::tapAdapterName = "Windscribe VPN";


const int typeIdConnectionAdapterInfo = qRegisterMetaType<ConnectionAdapterInfo>("ConnectionAdapterInfo");

void ConnectionAdapterInfo::outToLog()
{
    //todo
    qCDebug(LOG_CONNECTION) << "";
}
