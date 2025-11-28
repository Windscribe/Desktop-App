#include "location.h"

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

const int typeIdApiLocation = qRegisterMetaType<api_responses::Location>("apiinfo::Location");
const int typeIdApiLocationVector = qRegisterMetaType<QVector<api_responses::Location>>("QVector<apiinfo::Location>");

namespace api_responses {


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
    d->shortName_ = obj["short_name"].toString();
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
    WS_ASSERT(d->isValid_);
    QStringList ips;
    for (const Group &g : d->groups_)
    {
        ips << g.getPingIp();
    }
    return ips;
}

bool Location::operator ==(const Location &other) const
{
    return d->id_ == other.d->id_ &&
           d->name_ == other.d->name_ &&
           d->countryCode_ == other.d->countryCode_ &&
           d->shortName_ == other.d->shortName_ &&
           d->premiumOnly_ == other.d->premiumOnly_ &&
           d->p2p_ == other.d->p2p_ &&
           d->dnsHostName_ == other.d->dnsHostName_ &&
           d->groups_ == other.d->groups_ &&
           d->isValid_ == other.d->isValid_;
}

bool Location::operator !=(const Location &other) const
{
    return !operator==(other);
}

QDataStream& operator <<(QDataStream& stream, const Location& l)
{
    WS_ASSERT(l.d->isValid_);
    stream << l.versionForSerialization_;
    stream << l.d->id_ << l.d->name_ << l.d->countryCode_ << l.d->premiumOnly_ << l.d->p2p_ << l.d->dnsHostName_ << l.d->groups_ << l.d->shortName_;
    return stream;
}

QDataStream& operator >>(QDataStream& stream, Location& l)
{
    quint32 version;
    stream >> version;
    if (version > l.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        l.d->isValid_ = false;
        return stream;
    }
    stream >> l.d->id_ >> l.d->name_ >> l.d->countryCode_ >> l.d->premiumOnly_ >> l.d->p2p_ >> l.d->dnsHostName_ >> l.d->groups_;
    if (version >= 2) {
        stream >> l.d->shortName_;
    }
    l.d->isValid_ = true;

    return stream;
}

} // namespace api_responses
