#include "group.h"
#include <QJsonArray>

namespace ApiInfo {

bool Group::initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes)
{
    if (!obj.contains("id") || !obj.contains("city") || !obj.contains("nick") ||
            !obj.contains("pro") || !obj.contains("ping_ip"))
    {
        d->isValid_ = false;
        return false;
    }

    d->id_ = obj["id"].toInt();
    d->city_ = obj["city"].toString();
    d->nick_ = obj["nick"].toString();
    d->pro_ = obj["pro"].toInt();
    d->pingIp_ = obj["ping_ip"].toInt();

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
    return true;
}

QStringList Group::getAllIps() const
{
    Q_ASSERT(d->isValid_);

    QStringList list;
    for (const Node &node : d->nodes_)
    {
        list << node.getIp(0);
        list << node.getIp(1);
        list << node.getIp(2);
    }
    list << d->pingIp_;
    return list;
}

} // namespace ApiInfo
