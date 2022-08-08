#ifndef NETWORKWHITELISTSHARED_H
#define NETWORKWHITELISTSHARED_H

#include <QList>
#include <QString>
#include "types/networkinterface.h"

namespace NetworkWhiteListShared
{
    QList<QString> networkTrustTypes();
    QList<QString> networkTrustTypesWithoutForget();

    types::NetworkInterface networkInterfaceByFriendlyName(QString friendlyName);
}


#endif // NETWORKWHITELISTSHARED_H
