#include "group.h"
#include <QJsonArray>

namespace types {

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


} // namespace types
