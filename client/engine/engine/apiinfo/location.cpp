#include "location.h"

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

const int typeIdLocation = qRegisterMetaType<apiinfo::Location>("apiinfo::Location");
const int typeIdLocations = qRegisterMetaType<QVector<apiinfo::Location>>("QVector<apiinfo::Location>");

namespace apiinfo {


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
    d->type_ = SERVER_LOCATION_DEFAULT;
    return true;
}

void Location::initFromProtoBuf(const ProtoApiInfo::Location &l)
{
    d->id_ = l.id();
    d->name_ = QString::fromStdString(l.name());
    d->countryCode_ = QString::fromStdString(l.country_code());
    d->premiumOnly_ = l.premium_only();
    d->p2p_ = l.p2p();
    d->dnsHostName_ = QString::fromStdString(l.dns_hostname());

    for (int i = 0; i < l.groups_size(); ++i)
    {
        Group group;
        group.initFromProtoBuf(l.groups(i));
        d->groups_ << group;
    }

    d->type_ = SERVER_LOCATION_DEFAULT;
    d->isValid_ = true;
}

ProtoApiInfo::Location Location::getProtoBuf() const
{
    Q_ASSERT(d->isValid_);
    ProtoApiInfo::Location l;
    l.set_id(d->id_);
    l.set_name(d->name_.toStdString());
    l.set_country_code(d->countryCode_.toStdString());
    l.set_premium_only(d->premiumOnly_);
    l.set_p2p(d->p2p_);
    l.set_dns_hostname(d->dnsHostName_.toStdString());
    for (const Group &group : d->groups_)
    {
        *l.add_groups() = group.getProtoBuf();
    }
    return l;
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

} // namespace apiinfo
