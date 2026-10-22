#include "group.h"
#include <QJsonArray>
#include <wsnet/WSNetServerLocations.h>

namespace api_responses {

void Group::initFromWsnet(const wsnet::ServerGroup &src)
{
    d->id_          = src.id;
    d->city_        = QString::fromStdString(src.city);
    d->nick_        = QString::fromStdString(src.nick);
    d->status_      = src.status;
    d->premiumOnly_ = src.premiumOnly;
    d->pingIp_      = QString::fromStdString(src.pingIp);
    d->pingHost_    = QString::fromStdString(src.pingHost);
    d->wg_pubkey_   = QString::fromStdString(src.wgPubKey);
    d->ovpn_x509_   = QString::fromStdString(src.ovpnX509);
    d->link_speed_  = src.linkSpeed;
    d->netLoad_      = src.netLoad;
    d->p2p_         = src.p2p;
    d->dnsHostName_ = QString::fromStdString(src.dnsHostName);

    // A node the server list marks force-disconnect (fd=1) is being drained, so it is left out of
    // nodes_ entirely: nothing downstream can pick it for a connection, and a group left with none
    // of them reads as disabled, the same as a group that arrived without hosts. ServerList names
    // those nodes separately, for evacuating a connection that is already on one of them.
    if (d->status_ == 1) {
        for (const auto &n : src.nodes) {
            if (n.forceDisconnect) {
                continue;
            }
            Node node;
            node.initFromWsnet(n);
            d->nodes_ << node;
        }
    }

    d->isValid_ = true;
}

bool Group::operator==(const Group &other) const
{
    return d->id_ == other.d->id_ &&
           d->city_ == other.d->city_ &&
           d->nick_ == other.d->nick_ &&
           d->status_ == other.d->status_ &&
           d->premiumOnly_ == other.d->premiumOnly_ &&
           d->pingIp_ == other.d->pingIp_ &&
           d->pingHost_ == other.d->pingHost_ &&
           d->wg_pubkey_ == other.d->wg_pubkey_ &&
           d->ovpn_x509_ == other.d->ovpn_x509_ &&
           d->link_speed_ == other.d->link_speed_ &&
           d->netLoad_ == other.d->netLoad_ &&
           d->p2p_ == other.d->p2p_ &&
           d->dnsHostName_ == other.d->dnsHostName_ &&
           d->nodes_ == other.d->nodes_ &&
           d->isValid_ == other.d->isValid_;
}

bool Group::operator!=(const Group &other) const
{
    return !operator==(other);
}

} // namespace api_responses
