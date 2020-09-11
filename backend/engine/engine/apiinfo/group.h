#ifndef APIINFO_GROUP_H
#define APIINFO_GROUP_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QSharedDataPointer>
#include "node.h"

namespace ApiInfo {

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
    QStringList getAllIps() const;


private:
    QSharedDataPointer<GroupData> d;
};

} //namespace ApiInfo

#endif // APIINFO_GROUP_H
