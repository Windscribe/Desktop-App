#include "node.h"

namespace ApiInfo {

Node::Node() : weight_(0), legacy_(0), forceDisconnect_(0), isValid_(false)/*,
                           isCustomOvpnConfig_(false)*/
{

}

bool Node::initFromJson(QJsonObject &obj)
{
    if (!obj.contains("ip") || !obj.contains("ip2") || !obj.contains("ip3") || !obj.contains("hostname") || !obj.contains("weight"))
    {
        isValid_ = false;
        return false;
    }

    ip_[0] = obj["ip"].toString();
    ip_[1] = obj["ip2"].toString();
    ip_[2] = obj["ip3"].toString();
    hostname_ = obj["hostname"].toString();
    weight_ = obj["weight"].toInt();

    if (obj.contains("legacy"))
    {
        legacy_ = obj["legacy"].toInt();
    }
    else
    {
        legacy_ = 0;
    }

    if (obj.contains("force_disconnect"))
    {
        forceDisconnect_ = obj["force_disconnect"].toInt();
    }
    else
    {
        forceDisconnect_ = 0;
    }
    //isCustomOvpnConfig_ = false;
    isValid_ = true;
    return true;
}

/*void ServerNode::initFromCustomOvpnConfig(const QString &name, const QString &hostname, const QString &pathCustomOvpnConfig)
{
    hostname_ = hostname;
    cityname_ = name;
    weight_ = 1;
    legacy_ = 0;
    forceDisconnect_ = 0;
    ip_[0] = hostname;
    ip_[1] = hostname;
    ip_[2] = hostname;
    pathCustomOvpnConfig_ = pathCustomOvpnConfig;

    isCustomOvpnConfig_ = true;
    isValid_ = true;
}

void ServerNode::initFromStaticIpDescr(const StaticIpDescr &sid)
{
    ip_[0] = sid.nodeIP1;
    ip_[1] = sid.nodeIP2;
    ip_[2] = sid.nodeIP3;
    hostname_ = sid.hostname;
    dnsHostName_ = sid.dnsHostname;
    cityname_ = sid.cityName + "(" + sid.staticIp + ")";
    cityNameForShow_ = sid.cityName;
    weight_ = 1;
    legacy_ = 0;
    forceDisconnect_ = 0;
    staticIp_ = sid.staticIp;
    staticIpType_ = sid.type;
    staticIpPorts_ = sid.ports;
    username_ = sid.username;
    password_ = sid.password;
    isValid_ = true;
}

void ServerNode::writeToStream(QDataStream &stream) const
{
    Q_ASSERT(isValid_);
    stream << ip_[0];
    stream << ip_[1];
    stream << ip_[2];
    stream << hostname_;
    stream << weight_;
    stream << legacy_;
    stream << cityname_;
}

void ServerNode::readFromStream(QDataStream &stream)
{
    stream >> ip_[0];
    stream >> ip_[1];
    stream >> ip_[2];
    stream >> hostname_;
    stream >> weight_;
    stream >> legacy_;
    stream >> cityname_;

    isValid_ = true;
}

QString ServerNode::getCityName() const
{
    Q_ASSERT(isValid_);
    return cityname_;
}*/

QString Node::getHostname() const
{
    Q_ASSERT(isValid_);
    return hostname_;
}

bool Node::isForceDisconnect() const
{
    Q_ASSERT(isValid_);
    return forceDisconnect_ == 1;
}

QString Node::getIp(int ind) const
{
    Q_ASSERT(isValid_);
    Q_ASSERT(ind >= 0 && ind <= 2);
    return ip_[ind];
}

/*QString ServerNode::getIpForPing() const
{
    Q_ASSERT(isValid_);
    return ip_[1];
}

bool ServerNode::isLegacy() const
{
    Q_ASSERT(isValid_);
    return legacy_ == 1;
}

int ServerNode::getWeight() const
{
    Q_ASSERT(isValid_);
    return weight_;
}

QString ServerNode::getCustomOvpnConfigPath() const
{
    Q_ASSERT(isValid_);
    return pathCustomOvpnConfig_;
}

void ServerNode::setIpForCustomOvpnConfig(const QString &ip)
{
    Q_ASSERT(isValid_);
    Q_ASSERT(isCustomOvpnConfig_);
    ip_[0] = ip;
    ip_[1] = ip;
    ip_[2] = ip;
}

QString ServerNode::getStaticIp() const
{
    Q_ASSERT(isValid_);
    return staticIp_;
}

QString ServerNode::getStaticIpType() const
{
    Q_ASSERT(isValid_);
    return staticIpType_;
}

QString ServerNode::getCityNameForShow() const
{
    Q_ASSERT(isValid_);
    return cityNameForShow_;
}

QString ServerNode::getUsername() const
{
    Q_ASSERT(isValid_);
    return username_;
}

QString ServerNode::getPassword() const
{
    Q_ASSERT(isValid_);
    return password_;
}

QString ServerNode::getStaticIpDnsName() const
{
    Q_ASSERT(isValid_);
    return dnsHostName_;
}

StaticIpPortsVector ServerNode::getAllStaticIpIntPorts() const
{
    Q_ASSERT(isValid_);
    StaticIpPortsVector ret;
    Q_FOREACH(const StaticIpPortDescr &portDescr, staticIpPorts_)
    {
        ret << portDescr.intPort;
    }
    return ret;
}

bool ServerNode::isEqual(const ServerNode &sn) const
{
    Q_ASSERT(isValid_);
    return (ip_[0] == sn.ip_[0] && ip_[1] == sn.ip_[1] && ip_[2] == sn.ip_[2] &&
            hostname_ == sn.hostname_ && cityname_ == sn.cityname_ && weight_ == sn.weight_ && legacy_ == sn.legacy_ &&
            pathCustomOvpnConfig_ == sn.pathCustomOvpnConfig_ && isCustomOvpnConfig_ == sn.isCustomOvpnConfig_ &&
            username_ == sn.username_ && password_ == sn.password_ && staticIp_ == sn.staticIp_ && staticIpType_ == sn.staticIpType_ &&
            staticIpPorts_ == sn.staticIpPorts_ && cityNameForShow_ == sn.cityNameForShow_ && dnsHostName_ == sn.dnsHostName_);
}

bool ServerNode::isEqualIpsAndHostnameAndLegacy(const ServerNode &sn) const
{
    Q_ASSERT(isValid_);
    return (ip_[0] == sn.ip_[0] && ip_[1] == sn.ip_[1] && ip_[2] == sn.ip_[2] && hostname_ == sn.hostname_ && legacy_ == sn.legacy_);
}*/

} //namespace ApiInfo
