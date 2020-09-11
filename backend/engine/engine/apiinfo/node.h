#ifndef APIINFO_NODE_H
#define APIINFO_NODE_H

#include <QJsonObject>
#include <QStringList>
//#include "staticipslocation.h"

namespace ApiInfo {

class Node
{
public:
    explicit Node();

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
    // data from API
    QString ip_[3];
    QString hostname_;
    int weight_;
    int legacy_;   //1 or 0
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

} //namespace ApiInfo

#endif // APIINFO_NODE_H
