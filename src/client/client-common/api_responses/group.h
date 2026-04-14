#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QSharedDataPointer>
#include "node.h"
#include "utils/ws_assert.h"

namespace wsnet { struct ServerGroup; }

namespace api_responses {

class GroupData : public QSharedData
{
public:
    GroupData() : id_(0), status_(1), premiumOnly_(false), link_speed_(100), netLoad_(0), p2p_(0), isValid_(false) {}

    GroupData(const GroupData &other)
        : QSharedData(other),
          id_(other.id_),
          city_(other.city_),
          nick_(other.nick_),
          status_(other.status_),
          premiumOnly_(other.premiumOnly_),
          pingIp_(other.pingIp_),
          pingHost_(other.pingHost_),
          wg_pubkey_(other.wg_pubkey_),
          ovpn_x509_(other.ovpn_x509_),
          link_speed_(other.link_speed_),
          netLoad_(other.netLoad_),
          p2p_(other.p2p_),
          nodes_(other.nodes_),
          isValid_(other.isValid_) {}
    ~GroupData() {}

    int id_;
    QString city_;
    QString nick_;
    int status_;
    bool premiumOnly_;
    QString pingIp_;
    QString pingHost_;
    QString wg_pubkey_;
    QString ovpn_x509_;
    int link_speed_;
    int netLoad_;
    int p2p_;
    QString dnsHostName_;   // if not empty, then use this dns, overwise from parent Location

    QVector<Node> nodes_;

    // internal state
    bool isValid_;
};

// implicitly shared class Group
class Group
{

public:
    explicit Group() : d(new GroupData) {}
    Group(const Group &other) : d (other.d) {}

    void initFromWsnet(const wsnet::ServerGroup &src);

    int getId() const { WS_ASSERT(d->isValid_); return d->id_; }
    QString getCity() const { WS_ASSERT(d->isValid_); return d->city_; }
    QString getNick() const { WS_ASSERT(d->isValid_); return d->nick_; }
    bool isPremiumOnly() const { WS_ASSERT(d->isValid_); return d->premiumOnly_; }
    bool isDisabled() const { WS_ASSERT(d->isValid_); return d->nodes_.isEmpty(); }
    int status() const { WS_ASSERT(d->isValid_); return d->status_; }
    QString getPingIp() const { WS_ASSERT(d->isValid_); return d->pingIp_; }
    QString getPingHost() const { WS_ASSERT(d->isValid_); return d->pingHost_; }
    QString getWgPubKey() const { WS_ASSERT(d->isValid_); return d->wg_pubkey_; }
    QString getOvpnX509() const { WS_ASSERT(d->isValid_); return d->ovpn_x509_; }
    int getLinkSpeed() const { WS_ASSERT(d->isValid_); return d->link_speed_; }
    int getNetLoad() const { WS_ASSERT(d->isValid_); return d->netLoad_; }
    int getP2P() const { WS_ASSERT(d->isValid_); return d->p2p_; }

    int getNodesCount() const { WS_ASSERT(d->isValid_); return d->nodes_.count(); }
    const Node &getNode(int ind) const { WS_ASSERT(d->isValid_); return d->nodes_[ind]; }

    void setOverrideDnsHostName(const QString &dnsHostName) { d->dnsHostName_ = dnsHostName; }
    QString getDnsHostName() const { WS_ASSERT(d->isValid_); return d->dnsHostName_; }

    bool operator== (const Group &other) const;
    bool operator!= (const Group &other) const;

private:
    QSharedDataPointer<GroupData> d;
};

} //namespace api_responses
