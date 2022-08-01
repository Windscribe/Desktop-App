#ifndef APIINFO_STATICIPS_H
#define APIINFO_STATICIPS_H

#include <QSharedPointer>
#include <QString>
#include <QVector>

namespace apiinfo {

struct StaticIpPortDescr
{
    unsigned int extPort;
    unsigned int intPort;
};

bool operator==(const StaticIpPortDescr &l, const StaticIpPortDescr&r);

class StaticIpPortsVector : public QVector<unsigned int>
{
public:
    QString getAsStringWithDelimiters() const;
};

struct StaticIpDescr
{
    uint id;
    uint ipId;
    QString staticIp;
    QString type;     // dc = datacenter ip, res = residential ip
    QString name;
    QString countryCode;
    QString shortName;
    QString cityName;
    uint serverId;
    QString nodeIP1;
    QString nodeIP2;
    QString nodeIP3;
    QString hostname;
    QString dnsHostname;
    QString username;
    QString password;
    QString wgIp;
    QString wgPubKey;
    QString ovpnX509;
    QVector<StaticIpPortDescr> ports;

    const QString& getPingIp() const { return nodeIP1; }

    StaticIpPortsVector getAllStaticIpIntPorts() const
    {
        StaticIpPortsVector ret;
        for (const StaticIpPortDescr &portDescr : ports)
        {
            ret << portDescr.intPort;
        }
        return ret;
    }

    bool operator== (const StaticIpDescr &other) const
    {
        return id == other.id &&
               ipId == other.ipId &&
               staticIp == other.staticIp &&
               type == other.type &&
               name == other.name &&
               countryCode == other.countryCode &&
               shortName == other.shortName &&
               cityName == other.cityName &&
               serverId == other.serverId &&
               nodeIP1 == other.nodeIP1 &&
               nodeIP2 == other.nodeIP2 &&
               nodeIP3 == other.nodeIP3 &&
               hostname == other.hostname &&
               dnsHostname == other.dnsHostname &&
               username == other.username &&
               password == other.password &&
               wgIp == other.wgIp &&
               wgPubKey == other.wgPubKey &&
               ovpnX509 == other.ovpnX509 &&
               ports == other.ports;
    }

    bool operator!= (const StaticIpDescr &other) const
    {
        return !operator==(other);
    }

};

// internal data for StaticIps
class StaticIpsData : public QSharedData
{
public:
    StaticIpsData() {}
    StaticIpsData(const StaticIpsData &other)
        : QSharedData(other),
          deviceName_(other.deviceName_),
          ips_(other.ips_) {}
    ~StaticIpsData() {}

    QString deviceName_;
    QVector<StaticIpDescr> ips_;
};


// implicitly shared class StaticIps
class StaticIps
{
public:
    explicit StaticIps() : d(new StaticIpsData) {}
    StaticIps(const StaticIps &other) : d (other.d) {}

    bool initFromJson(QJsonObject &obj);

    const QString &getDeviceName() const { return d->deviceName_; }
    int getIpsCount() const { return d->ips_.count(); }
    const StaticIpDescr &getIp(int ind) const { Q_ASSERT(ind >= 0 && ind < d->ips_.count()); return d->ips_[ind]; }

    QStringList getAllPingIps() const;

    bool operator== (const StaticIps &other) const
    {
        return d->deviceName_ == other.d->deviceName_ &&
               d->ips_ == other.d->ips_;
    }

    bool operator!= (const StaticIps &other) const
    {
        return !operator==(other);
    }

    StaticIps& operator=(const StaticIps&) = default;

private:
    QSharedDataPointer<StaticIpsData> d;
};

} //namespace apiinfo

#endif // APIINFO_STATICIPS_H
