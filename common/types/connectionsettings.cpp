#include "connectionsettings.h"
#include "utils/logger.h"

namespace types {

void ConnectionSettings::debugToLog() const
{
    qCDebug(LOG_BASIC) << "Connection settings automatic:" << isAutomatic;
    qCDebug(LOG_BASIC) << "Connection settings protocol:" << protocol.toLongString();
    qCDebug(LOG_BASIC) << "Connection settings port:" << port;
}

void ConnectionSettings::logConnectionSettings() const
{
    if (isAutomatic)
    {
        qCDebug(LOG_CONNECTION) << "Connection settings: automatic";
    }
    else
    {
        qCDebug(LOG_CONNECTION) << "Connection settings: manual " << protocol.toShortString() << port;
    }
}

QDebug operator<<(QDebug dbg, const ConnectionSettings &cs)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{isAutomatic:" << cs.isAutomatic << "; ";
    dbg << "protocol:" << cs.protocol.toLongString() << "; ";
    dbg << "port:" << cs.port << "}";
    return dbg;
}

} //namespace types

