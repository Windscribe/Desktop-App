#ifndef APIINFO_NODE_H
#define APIINFO_NODE_H

#include <QJsonObject>
#include <QSharedDataPointer>
#include <QStringList>

namespace ApiInfo {

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

    // for custom ovpn config
    /*bool isCustomOvpnConfig_;
    QString pathCustomOvpnConfig_;

    // for staticIps
    QString staticIp_;
    QString staticIpType_;
    QVector<StaticIpPortDescr> staticIpPorts_;
    QString cityNameForShow_;
    QString dnsHostName_;
    // credentials for staticIps
    QString username_;
    QString password_;*/
};

// implicitly shared class Node
class Node
{
public:
    explicit Node() { d = new NodeData; }
    Node(const Node &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj);

    /*void initFromCustomOvpnConfig(const QString &name, const QString &hostname,
                                  const QString &pathCustomOvpnConfig);

    void initFromStaticIpDescr(const StaticIpDescr &sid);

    void writeToStream(QDataStream &stream) const;
    void readFromStream(QDataStream &stream);

    QString getCityName() const;*/
    QString getHostname() const;
    bool isForceDisconnect() const;
    QString getIp(int ind) const;
    /*QString getIpForPing() const;
    bool isLegacy() const;
    int getWeight() const;

    QString getCustomOvpnConfigPath() const;
    void setIpForCustomOvpnConfig(const QString &ip);

    QString getStaticIp() const;
    QString getStaticIpType() const;
    QString getCityNameForShow() const;
    QString getUsername() const;
    QString getPassword() const;
    QString getStaticIpDnsName() const;

    StaticIpPortsVector getAllStaticIpIntPorts() const;

    bool isEqual(const ServerNode &sn) const;
    bool isEqualIpsAndHostnameAndLegacy(const ServerNode &sn) const;*/

private:
    QSharedDataPointer<NodeData> d;
};

} //namespace ApiInfo

#endif // APIINFO_NODE_H
