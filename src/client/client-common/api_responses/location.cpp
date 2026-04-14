#include "location.h"

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <wsnet/WSNetServerLocations.h>

const int typeIdApiLocation = qRegisterMetaType<api_responses::Location>("apiinfo::Location");
const int typeIdApiLocationVector = qRegisterMetaType<QVector<api_responses::Location>>("QVector<apiinfo::Location>");

namespace api_responses {


void Location::initFromWsnet(const wsnet::ServerLocation &src)
{
    d->id_          = src.id;
    d->name_        = QString::fromStdString(src.name);
    d->countryCode_ = QString::fromStdString(src.countryCode);
    d->shortName_   = QString::fromStdString(src.shortName);
    d->premiumOnly_ = src.premiumOnly;
    d->dnsHostName_ = QString::fromStdString(src.dnsHostName);

    for (const auto &g : src.groups) {
        Group group;
        group.initFromWsnet(g);
        if (g.status != 0) {
            d->groups_ << group;
        }
    }

    d->isValid_ = true;
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
           d->dnsHostName_ == other.d->dnsHostName_ &&
           d->groups_ == other.d->groups_ &&
           d->isValid_ == other.d->isValid_;
}

bool Location::operator !=(const Location &other) const
{
    return !operator==(other);
}

} // namespace api_responses
