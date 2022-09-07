#include "networkoptionsshared.h"
#include <QObject>

#include "backend/persistentstate.h"

namespace NetworkOptionsShared
{

types::NetworkInterface networkInterfaceByName(QString networkOrSsid)
{
    QVector<types::NetworkInterface> list = PersistentState::instance().networkWhitelist();
    for (types::NetworkInterface interface : list)
    {
        if (interface.networkOrSsid == networkOrSsid)
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