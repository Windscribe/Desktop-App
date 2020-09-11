#include "group.h"
#include <QJsonArray>

namespace ApiInfo {

Group::Group() : id_(0), pro_(0), isValid_(false)
{

}

bool Group::initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes)
{
    if (!obj.contains("id") || !obj.contains("city") || !obj.contains("nick") ||
            !obj.contains("pro") || !obj.contains("ping_ip"))
    {
        isValid_ = false;
        return false;
    }

    id_ = obj["id"].toInt();
    city_ = obj["city"].toString();
    nick_ = obj["nick"].toString();
    pro_ = obj["pro"].toInt();
    pingIp_ = obj["ping_ip"].toInt();

    if (obj.contains("nodes"))
    {
        const auto nodesArray = obj["nodes"].toArray();
        for (const QJsonValue &serverNodeValue : nodesArray)
        {
            QJsonObject objServerNode = serverNodeValue.toObject();
            Node node;
            if (!node.initFromJson(objServerNode))
            {
                isValid_ = false;
                return false;
            }

            // not add node with flag force_diconnect, but add it to another list
            if (node.isForceDisconnect())
            {
                forceDisconnectNodes << node.getHostname();
            }
            else
            {
                nodes_ << node;
            }
        }
    }
    return true;
}

QStringList Group::getAllIps() const
{
    Q_ASSERT(isValid_);

    QStringList list;
    for (const Node &node : nodes_)
    {
        list << node.getIp(0);
        list << node.getIp(1);
        list << node.getIp(2);
    }
    list << pingIp_;
    return list;
}

} // namespace ApiInfo
