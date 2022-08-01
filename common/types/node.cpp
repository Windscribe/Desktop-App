#include "node.h"

namespace types {

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

} //namespace types
