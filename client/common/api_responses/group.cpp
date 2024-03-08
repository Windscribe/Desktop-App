#include "group.h"
#include <QJsonArray>
#include "utils/ws_assert.h"

namespace api_responses {

bool Group::initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes)
{
    if (!obj.contains("id") || !obj.contains("city") || !obj.contains("nick") ||
            !obj.contains("pro") || !obj.contains("ping_ip") || !obj.contains("wg_pubkey"))
    {
        d->isValid_ = false;
        return false;
    }

    d->id_ = obj["id"].toInt();
    d->city_ = obj["city"].toString();
    d->nick_ = obj["nick"].toString();
    d->pro_ = obj["pro"].toInt();
    d->pingIp_ = obj["ping_ip"].toString();
    d->pingHost_ = obj["ping_host"].toString();
    d->wg_pubkey_ = obj["wg_pubkey"].toString();
    d->ovpn_x509_ = obj["ovpn_x509"].toString();

    if (obj.contains("link_speed"))
    {
        bool bConverted;
        d->link_speed_ = obj.value("link_speed").toString().toInt(&bConverted);
        if (!bConverted) {
            d->link_speed_ = 100;
        }
    }

    // Using -1 to indicate to the UI logic that the load (health) value was invalid/missing,
    // and therefore this location should be excluded when calculating the region's average
    // load value.
    // Note: the server json does not include a health value for premium locations when the
    // user is logged into a free account.
    if (obj.contains("health"))
    {
        d->health_ = obj.value("health").toInt(-1);
        if ((d->health_ < 0) || (d->health_ > 100)) {
            d->health_ = -1;
        }
    }
    else {
        d->health_ = -1;
    }

    if (obj.contains("nodes"))
    {
        const auto nodesArray = obj["nodes"].toArray();
        for (const QJsonValue &serverNodeValue : nodesArray)
        {
            QJsonObject objServerNode = serverNodeValue.toObject();
            Node node;
            if (!node.initFromJson(objServerNode))
            {
                d->isValid_ = false;
                return false;
            }

            // not add node with flag force_diconnect, but add it to another list
            if (node.isForceDisconnect())
            {
                forceDisconnectNodes << node.getHostname();
            }
            else
            {
                d->nodes_ << node;
            }
        }
    }
    d->isValid_ = true;
    return true;
}

bool Group::operator==(const Group &other) const
{
    return d->id_ == other.d->id_ &&
           d->city_ == other.d->city_ &&
           d->nick_ == other.d->nick_ &&
           d->pro_ == other.d->pro_ &&
           d->pingIp_ == other.d->pingIp_ &&
           d->pingHost_ == other.d->pingHost_ &&
           d->wg_pubkey_ == other.d->wg_pubkey_ &&
           d->ovpn_x509_ == other.d->ovpn_x509_ &&
           d->link_speed_ == other.d->link_speed_ &&
           d->health_ == other.d->health_ &&
           d->dnsHostName_ == other.d->dnsHostName_ &&
           d->nodes_ == other.d->nodes_ &&
           d->isValid_ == other.d->isValid_;
}

bool Group::operator!=(const Group &other) const
{
    return !operator==(other);
}

QDataStream& operator <<(QDataStream& stream, const Group& g)
{
    WS_ASSERT(g.d->isValid_);
    stream << g.versionForSerialization_;
    stream << g.d->id_ << g.d->city_ << g.d->nick_ << g.d->pro_ << g.d->pingIp_ << g.d->pingHost_ << g.d->wg_pubkey_ << g.d->ovpn_x509_ << g.d->link_speed_ <<
              g.d->health_ << g.d->dnsHostName_ << g.d->nodes_;

    return stream;
}

QDataStream& operator >>(QDataStream& stream, Group& g)
{
    quint32 version;
    stream >> version;
    if (version > g.versionForSerialization_) {
        stream.setStatus(QDataStream::ReadCorruptData);
        g.d->isValid_ = false;
        return stream;
    }

    if (version == 1) {
        stream >> g.d->id_ >> g.d->city_ >> g.d->nick_ >> g.d->pro_ >> g.d->pingIp_ >> g.d->wg_pubkey_ >> g.d->ovpn_x509_ >> g.d->link_speed_ >>
                  g.d->health_ >> g.d->dnsHostName_ >> g.d->nodes_;
    } else if (version == g.versionForSerialization_) {
        stream >> g.d->id_ >> g.d->city_ >> g.d->nick_ >> g.d->pro_ >> g.d->pingIp_ >> g.d->pingHost_ >> g.d->wg_pubkey_ >> g.d->ovpn_x509_ >> g.d->link_speed_ >>
                  g.d->health_ >> g.d->dnsHostName_ >> g.d->nodes_;
    }

    g.d->isValid_ = true;

    return stream;
}


} // namespace api_responses
