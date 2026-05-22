#include "node.h"
#include "utils/ws_assert.h"
#include <QString>
#include <wsnet/WSNetServerLocations.h>

namespace api_responses {

void Node::initFromWsnet(const wsnet::ServerNode &src)
{
    d->ips_ = { QString::fromStdString(src.ip),
                QString::fromStdString(src.ip2),
                QString::fromStdString(src.ip3) };
    d->host_ = QString::fromStdString(src.host);
    d->weight_   = src.weight;
    d->ipv6_ = (src.ipv6 != 0);
    d->isValid_  = true;
}

QString Node::getHost() const
{
    WS_ASSERT(d->isValid_);
    return d->host_;
}

QString Node::getIp(int ind) const
{
    WS_ASSERT(d->isValid_);
    WS_ASSERT(ind >= 0 && ind <= 2);
    WS_ASSERT(d->ips_.size() == 3);
    return d->ips_[ind];
}

int Node::getWeight() const
{
    WS_ASSERT(d->isValid_);
    return d->weight_;
}

bool Node::isIpv6Support() const
{
    return d->ipv6_;
}

bool Node::operator==(const Node &other) const
{
    return d->ips_ == other.d->ips_ &&
           d->host_ == other.d->host_ &&
           d->weight_ == other.d->weight_ &&
           d->ipv6_ == other.d->ipv6_ &&
           d->isValid_ == other.d->isValid_;
}

bool Node::operator!=(const Node &other) const
{
    return !operator==(other);
}

} //namespace api_responses
