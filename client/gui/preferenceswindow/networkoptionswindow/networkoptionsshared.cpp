#include "networkoptionsshared.h"
#include <QObject>

#include "backend/persistentstate.h"

namespace NetworkOptionsShared
{

types::NetworkInterface networkInterfaceByFriendlyName(QString friendlyName)
{
    QVector<types::NetworkInterface> list = PersistentState::instance().networkWhitelist();
    for (types::NetworkInterface interface : list)
    {
        if (interface.friendlyName == friendlyName)
        {
            return interface;
        }
    }
    return {};
}

const char *trustTypeToString(NETWORK_TRUST_TYPE type)
{
    switch(type)
    {
        case NETWORK_TRUST_SECURED:
            return "Secured";
        case NETWORK_TRUST_UNSECURED:
            return "Unsecured";
        default:
            return "";
    }
}

}