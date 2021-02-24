#include "networkwhitelistshared.h"
#include <QObject>

#include "../backend/persistentstate.h"

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


ProtoTypes::NetworkInterface networkInterfaceByFriendlyName(QString friendlyName)
{
    ProtoTypes::NetworkInterface network;

    QString foundNetworkId = "";
    ProtoTypes::NetworkWhiteList networkListOld = PersistentState::instance().networkWhitelist();
    for (int i = 0; i < networkListOld.networks_size(); i++)
    {
        if (networkListOld.networks(i).friendly_name() == friendlyName.toStdString())
        {
            network.set_friendly_name(friendlyName.toStdString());
            network.set_interface_type(networkListOld.networks(i).interface_type());
            network.set_network_or_ssid(networkListOld.networks(i).network_or_ssid());
            network.set_trust_type(networkListOld.networks(i).trust_type());
            network.set_active(networkListOld.networks(i).active());
            break;
        }
    }

    return network;
}

}
