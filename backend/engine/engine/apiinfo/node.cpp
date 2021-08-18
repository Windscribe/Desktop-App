#include "node.h"

namespace apiinfo {

bool Node::initFromJson(QJsonObject &obj)
{
    if (!obj.contains("ip") || !obj.contains("ip2") || !obj.contains("ip3") || !obj.contains("hostname") || !obj.contains("weight"))
    {
        d->isValid_ = false;
        return false;
    }

    d->ip_[0] = obj["ip"].toString();
    d->ip_[1] = obj["ip2"].toString();
    d->ip_[2] = obj["ip3"].toString();
    d->hostname_ = obj["hostname"].toString();
    d->weight_ = obj["weight"].toInt();

    if (obj.contains("force_disconnect"))
    {
        d->forceDisconnect_ = obj["force_disconnect"].toInt();
    }
    else
    {
        d->forceDisconnect_ = 0;
    }

    d->isValid_ = true;
    return true;
}

void Node::initFromProtoBuf(const ProtoApiInfo::Node &n)
{
    Q_ASSERT(n.ips_size() == 3);
    if (n.ips_size() == 3)
    {
        for (auto i = 0; i < 3; ++i)
        {
            d->ip_[i] = QString::fromStdString(n.ips(i));
        }
    }

    d->hostname_ = QString::fromStdString(n.hostname());
    d->weight_ = n.weight();
    d->forceDisconnect_ = false;
    d->isValid_ = true;
}

ProtoApiInfo::Node Node::getProtoBuf() const
{
    Q_ASSERT(d->isValid_);
    ProtoApiInfo::Node node;

    for (auto i = 0; i < 3; ++i)
    {
        *node.add_ips() = d->ip_[i].toStdString();
    }
    node.set_hostname(d->hostname_.toStdString());
    node.set_weight(d->weight_);
    return node;
}

QString Node::getHostname() const
{
    Q_ASSERT(d->isValid_);
    return d->hostname_;
}

bool Node::isForceDisconnect() const
{
    Q_ASSERT(d->isValid_);
    return d->forceDisconnect_ == 1;
}

QString Node::getIp(int ind) const
{
    Q_ASSERT(d->isValid_);
    Q_ASSERT(ind >= 0 && ind <= 2);
    return d->ip_[ind];
}

int Node::getWeight() const
{
    Q_ASSERT(d->isValid_);
    return d->weight_;
}

} //namespace apiinfo
