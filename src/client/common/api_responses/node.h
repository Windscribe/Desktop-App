#pragma once

#include <QJsonObject>
#include <QSharedDataPointer>
#include <QStringList>

namespace api_responses {

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
    Node() : d(new NodeData) {}

    bool initFromJson(QJsonObject &obj);

    QString getHostname() const;
    bool isForceDisconnect() const;
    QString getIp(int ind) const;
    int getWeight() const;

    bool operator== (const Node &other) const;
    bool operator!= (const Node &other) const;

    friend QDataStream& operator <<(QDataStream &stream, const Node &n);
    friend QDataStream& operator >>(QDataStream &stream, Node &n);

private:
    QSharedDataPointer<NodeData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace api_responses

