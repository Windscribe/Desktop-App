#ifndef STATICIPSLOCATION_H
#define STATICIPSLOCATION_H

#include <QSharedPointer>
#include <QString>
#include <QVector>

class ServerLocation;


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

class StaticIpsLocation
{
public:
    StaticIpsLocation();

    bool initFromJson(QJsonObject &obj);


    void writeToStream(QDataStream &stream) const;
    void readFromStream(QDataStream &stream, int revision);

    QSharedPointer<ServerLocation> makeServerLocation();

private:
    static constexpr int REVISION_VERSION = 1;

    QString deviceName_;
    QVector<StaticIpDescr> ips_;
};

#endif // STATICIPSLOCATION_H
