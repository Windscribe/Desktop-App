#ifndef APIINFO_NODE_H
#define APIINFO_NODE_H

#include <QJsonObject>
#include <QSharedDataPointer>
#include <QStringList>
#include "ipc/generated_proto/apiinfo.pb.h"

namespace apiinfo {

class NodeData : public QSharedData
{
public:
    NodeData() : weight_(0), forceDisconnect_(0), isValid_(false) {}

    NodeData(const NodeData &other)
        : QSharedData(other),
          hostname_(other.hostname_),
          weight_(other.weight_),
          forceDisconnect_(other.forceDisconnect_),
          isValid_(other.isValid_)
    {
        ip_[0] = other.ip_[0];
        ip_[1] = other.ip_[1];
        ip_[2] = other.ip_[2];
    }
    ~NodeData() {}

    // data from API
    QString ip_[3];
    QString hostname_;
    int weight_;
    int forceDisconnect_;

    // internal state
    bool isValid_;
};

// implicitly shared class Node
class Node
{
public:
    explicit Node() { d = new NodeData; }
    Node(const Node &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj);
    void initFromProtoBuf(const ProtoApiInfo::Node &n);
    ProtoApiInfo::Node getProtoBuf() const;

    QString getHostname() const;
    bool isForceDisconnect() const;
    QString getIp(int ind) const;
    int getWeight() const;

    bool operator== (const Node &other) const
    {
        return d->ip_[0] == other.d->ip_[0] &&
               d->ip_[1] == other.d->ip_[1] &&
               d->ip_[2] == other.d->ip_[2] &&
               d->hostname_ == other.d->hostname_ &&
               d->weight_ == other.d->weight_ &&
               d->forceDisconnect_ == other.d->forceDisconnect_ &&
               d->isValid_ == other.d->isValid_;
    }

    bool operator!= (const Node &other) const
    {
        return !operator==(other);
    }

private:
    QSharedDataPointer<NodeData> d;
};

} //namespace apiinfo

#endif // APIINFO_NODE_H
