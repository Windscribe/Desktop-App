#ifndef APIINFO_STATICIPS_H
#define APIINFO_STATICIPS_H

#include <QSharedPointer>
#include <QString>
#include <QVector>
#include "ipc/generated_proto/apiinfo.pb.h"

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
    QVector<StaticIpPortDescr> ports;

    const QString& getPingIp() const { return nodeIP2; }

    StaticIpPortsVector getAllStaticIpIntPorts() const
    {
        StaticIpPortsVector ret;
        for (const StaticIpPortDescr &portDescr : ports)
        {
            ret << portDescr.intPort;
        }
        return ret;

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
    explicit StaticIps() { d = new StaticIpsData; }
    StaticIps(const StaticIps &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj);
    void initFromProtoBuf(const ProtoApiInfo::StaticIps &staticIps);
    ProtoApiInfo::StaticIps getProtoBuf() const;

    const QString &getDeviceName() const { return d->deviceName_; }
    int getIpsCount() const { return d->ips_.count(); }
    const StaticIpDescr &getIp(int ind) const { Q_ASSERT(ind >= 0 && ind < d->ips_.count()); return d->ips_[ind]; }

    QStringList getAllIps() const;

private:
    QSharedDataPointer<StaticIpsData> d;
};

} //namespace apiinfo

#endif // APIINFO_STATICIPS_H
