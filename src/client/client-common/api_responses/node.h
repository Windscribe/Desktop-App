#pragma once

#include <QJsonObject>
#include <QSharedDataPointer>
#include <QStringList>

namespace wsnet { struct ServerNode; }

namespace api_responses {

class NodeData : public QSharedData
{
public:
    NodeData() : weight_(0), isValid_(false) {}
    ~NodeData() {}

    // data from API
    QVector<QString> ips_;   // 3 ips
    QString host_;
    int weight_;

    // internal state
    bool isValid_;
};

// implicitly shared class Node
class Node
{
public:
    Node() : d(new NodeData) {}

    void initFromWsnet(const wsnet::ServerNode &src);

    QString getHost() const;
    QString getIp(int ind) const;
    int getWeight() const;

    bool operator== (const Node &other) const;
    bool operator!= (const Node &other) const;

private:
    QSharedDataPointer<NodeData> d;
};

} //namespace api_responses

