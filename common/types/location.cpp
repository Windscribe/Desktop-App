#include "location.h"

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

const int typeIdLocation = qRegisterMetaType<types::Location>("types::Location");
const int typeIdLocations = qRegisterMetaType<QVector<types::Location>>("QVector<types::Location>");

namespace types {


bool Location::initFromJson(const QJsonObject &obj, QStringList &forceDisconnectNodes)
{
    if (!obj.contains("id") || !obj.contains("name") || !obj.contains("country_code") ||
            !obj.contains("premium_only") || !obj.contains("p2p") || !obj.contains("groups"))
    {
        d->isValid_ = false;
        return false;
    }

    d->id_ = obj["id"].toInt();
    d->name_ = obj["name"].toString();
    d->countryCode_ = obj["country_code"].toString();
    d->premiumOnly_ = obj["premium_only"].toInt();
    d->p2p_ = obj["p2p"].toInt();
    if (obj.contains("dns_hostname"))
    {
        d->dnsHostName_ = obj["dns_hostname"].toString();
    }

    const auto groupsArray = obj["groups"].toArray();
    for (const QJsonValue &serverGroupValue : groupsArray)
    {
        QJsonObject objServerGroup = serverGroupValue.toObject();

        Group group;
        if (!group.initFromJson(objServerGroup, forceDisconnectNodes))
        {
            d->isValid_ = false;
            return false;
        }
        d->groups_ << group;
    }

    d->isValid_ = true;
    return true;
}

QStringList Location::getAllPingIps() const
{
    Q_ASSERT(d->isValid_);
    QStringList ips;
    for (const Group &g : d->groups_)
    {
        ips << g.getPingIp();
    }
    return ips;
}

} // namespace types
