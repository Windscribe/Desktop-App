#ifndef NETWORKWHITELISTSHARED_H
#define NETWORKWHITELISTSHARED_H

#include <QList>
#include <QString>
#include "utils/protobuf_includes.h"

namespace NetworkWhiteListShared
{
    QList<QString> networkTrustTypes();
    QList<QString> networkTrustTypesWithoutForget();

    ProtoTypes::NetworkInterface networkInterfaceByFriendlyName(QString friendlyName);
}


#endif // NETWORKWHITELISTSHARED_H
