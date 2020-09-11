#ifndef APIINFO_GROUP_H
#define APIINFO_GROUP_H

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "node.h"

namespace ApiInfo {

class Group
{

public:
    explicit Group();

    bool initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes);
    QStringList getAllIps() const;


private:
    int id_;
    QString city_;
    QString nick_;
    int pro_;       // 0 - for free account, 1 - for pro account
    QString pingIp_;

    QVector<Node> nodes_;

    // internal state
    bool isValid_;
};

} //namespace ApiInfo

#endif // APIINFO_GROUP_H
