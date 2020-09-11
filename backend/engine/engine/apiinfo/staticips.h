#ifndef APIINFO_STATICIPS_H
#define APIINFO_STATICIPS_H

#include <QSharedPointer>
#include <QString>
#include <QVector>

namespace ApiInfo {


struct StaticIpPortDescr
{
    unsigned int extPort;
    unsigned int intPort;
};

bool operator==(const StaticIpPortDescr &l, const StaticIpPortDescr&r);


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
    QVector<StaticIpPortDescr> ports;
    QString username;
    QString password;
};

class StaticIpPortsVector : public QVector<unsigned int>
{
public:
    QString getAsStringWithDelimiters() const;
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


    //void writeToStream(QDataStream &stream) const;
    //void readFromStream(QDataStream &stream, int revision);

    //QSharedPointer<ServerLocation> makeServerLocation();

private:
    QSharedDataPointer<StaticIpsData> d;
};

} //namespace ApiInfo

#endif // APIINFO_STATICIPS_H
