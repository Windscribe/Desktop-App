#ifndef TYPES_NODE_H
#define TYPES_NODE_H

#include <QJsonObject>
#include <QSharedDataPointer>
#include <QStringList>

namespace types {

class NodeData : public QSharedData
{
public:
    NodeData() : weight_(0), forceDisconnect_(0), isValid_(false) {}
    ~NodeData() {}

    // data from API
    QVector<QString> ips_;   // 3 ips
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
    explicit Node() : d(new NodeData) {}
    Node(const Node &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj);

    QString getHostname() const;
    bool isForceDisconnect() const;
    QString getIp(int ind) const;
    int getWeight() const;

    bool operator== (const Node &other) const
    {
        return d->ips_ == other.d->ips_ &&
               d->hostname_ == other.d->hostname_ &&
               d->weight_ == other.d->weight_ &&
               d->forceDisconnect_ == other.d->forceDisconnect_ &&
               d->isValid_ == other.d->isValid_;
    }

    bool operator!= (const Node &other) const
    {
        return !operator==(other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const Node &n)
    {
        Q_ASSERT(n.d->isValid_);
        stream << versionForSerialization_;

        // forceDisconnect_ does not require serialization
        stream << n.d->ips_ << n.d->hostname_ << n.d->weight_;

        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, Node &n)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version != versionForSerialization_)
        {
            n.d->isValid_ = false;
            return stream;
        }

        stream >> n.d->ips_ >> n.d->hostname_ >> n.d->weight_;
        n.d->isValid_ = true;

        return stream;
    }

private:
    QSharedDataPointer<NodeData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_NODE_H
