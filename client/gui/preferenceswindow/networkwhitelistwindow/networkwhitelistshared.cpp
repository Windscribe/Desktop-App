#include "networkwhitelistshared.h"
#include <QObject>

#include "backend/persistentstate.h"

namespace NetworkWhiteListShared
{

QList<QString> networkTrustTypes()
{
    QList<QString> selections;
    selections.append(QObject::tr("Secured"));
    selections.append(QObject::tr("Unsecured"));
    selections.append(QObject::tr("Forget"));
    return selections;
}

QList<QString> networkTrustTypesWithoutForget()
{
    QList<QString> selections;
    selections.append(QObject::tr("Secured"));
    selections.append(QObject::tr("Unsecured"));
    return selections;
}


types::NetworkInterface networkInterfaceByFriendlyName(QString friendlyName)
{
    types::NetworkInterface network;

    QVector<types::NetworkInterface> networkListOld = PersistentState::instance().networkWhitelist();
    for (int i = 0; i < networkListOld.size(); i++)
    {
        if (networkListOld[i].friendlyName == friendlyName)
        {
            network.friendlyName = friendlyName;
            network.interfaceType = networkListOld[i].interfaceType;
            network.networkOrSSid =  networkListOld[i].networkOrSSid;
            network.trustType = networkListOld[i].trustType;
            network.active = networkListOld[i].active;
            break;
        }
    }

    return network;
}

}
