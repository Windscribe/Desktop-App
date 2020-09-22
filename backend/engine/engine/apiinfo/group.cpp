#include "group.h"
#include <QJsonArray>

namespace apiinfo {

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
    d->pingIp_ = obj["ping_ip"].toString();

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

void Group::initFromProtoBuf(const ProtoApiInfo::Group &g)
{
    d->id_ = g.id();
    d->city_ = QString::fromStdString(g.city());
    d->nick_ = QString::fromStdString(g.nick());
    d->pro_ = g.pro();
    d->pingIp_ = QString::fromStdString(g.ping_ip());

    for (int i = 0; i < g.nodes_size(); ++i)
    {
        Node node;
        node.initFromProtoBuf(g.nodes(i));
        d->nodes_ << node;
    }

    d->isValid_ = true;
}

ProtoApiInfo::Group Group::getProtoBuf() const
{
    Q_ASSERT(d->isValid_);

    ProtoApiInfo::Group group;
    group.set_id(d->id_);
    group.set_city(d->city_.toStdString());
    group.set_nick(d->nick_.toStdString());
    group.set_pro(d->pro_);
    group.set_ping_ip(d->pingIp_.toStdString());
    for (const Node &node : d->nodes_)
    {
        *group.add_nodes() = node.getProtoBuf();
    }
    return group;
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

} // namespace apiinfo
