#ifndef APIINFO_GROUP_H
#define APIINFO_GROUP_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QSharedDataPointer>
#include "node.h"
#include "ipc/generated_proto/apiinfo.pb.h"

namespace apiinfo {

class GroupData : public QSharedData
{
public:
    GroupData() : id_(0), pro_(0), isValid_(false) {}

    GroupData(const GroupData &other)
        : QSharedData(other),
          id_(other.id_),
          city_(other.city_),
          nick_(other.nick_),
          pro_(other.pro_),
          pingIp_(other.pingIp_),
          nodes_(other.nodes_),
          isValid_(other.isValid_) {}
    ~GroupData() {}

    int id_;
    QString city_;
    QString nick_;
    int pro_;       // 0 - for free account, 1 - for pro account
    QString pingIp_;
    QString wg_pubkey_;

    QVector<Node> nodes_;

    // internal state
    bool isValid_;
};

// implicitly shared class Group
class Group
{

public:
    explicit Group() { d = new GroupData; }
    Group(const Group &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes);
    void initFromProtoBuf(const ProtoApiInfo::Group &g);
    ProtoApiInfo::Group getProtoBuf() const;

    int getId() const { Q_ASSERT(d->isValid_); return d->id_; }
    QString getCity() const { Q_ASSERT(d->isValid_); return d->city_; }
    QString getNick() const { Q_ASSERT(d->isValid_); return d->nick_; }
    bool isPro() const { Q_ASSERT(d->isValid_); return d->pro_ != 0; }
    bool isDisabled() const { Q_ASSERT(d->isValid_); return d->nodes_.isEmpty(); }
    QString getPingIp() const { Q_ASSERT(d->isValid_); return d->pingIp_; }
    QString getWgPubKey() const { Q_ASSERT(d->isValid_); return d->wg_pubkey_; }

    int getNodesCount() const { Q_ASSERT(d->isValid_); return d->nodes_.count(); }
    const Node &getNode(int ind) const { Q_ASSERT(d->isValid_); return d->nodes_[ind]; }


    bool operator== (const Group &other) const
    {
        return d->id_ == other.d->id_ &&
               d->city_ == other.d->city_ &&
               d->nick_ == other.d->nick_ &&
               d->pro_ == other.d->pro_ &&
               d->pingIp_ == other.d->pingIp_ &&
               d->wg_pubkey_ == other.d->wg_pubkey_ &&
               d->nodes_ == other.d->nodes_ &&
               d->isValid_ == other.d->isValid_;
    }

    bool operator!= (const Group &other) const
    {
        return !operator==(other);
    }

private:
    QSharedDataPointer<GroupData> d;
};

} //namespace apiinfo

#endif // APIINFO_GROUP_H
