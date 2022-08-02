#ifndef TYPES_STATICIPS_H
#define TYPES_STATICIPS_H

#include <QSharedPointer>
#include <QString>
#include <QVector>

namespace types {

struct StaticIpPortDescr
{
    unsigned int extPort;
    unsigned int intPort;

    friend QDataStream& operator <<(QDataStream& stream, const StaticIpPortDescr& s)
    {
        stream << versionForSerialization_;
        stream << s.extPort << s.intPort;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, StaticIpPortDescr& s)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        stream >> s.extPort >> s.intPort;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;
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
    QVector<QString> nodeIPs;    // 3 ips
    QString hostname;
    QString dnsHostname;
    QString username;
    QString password;
    QString wgIp;
    QString wgPubKey;
    QString ovpnX509;
    QVector<StaticIpPortDescr> ports;

    const QString& getPingIp() const { Q_ASSERT(!nodeIPs.isEmpty()); return nodeIPs[0]; }

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
               nodeIPs == other.nodeIPs &&
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

    friend QDataStream& operator <<(QDataStream& stream, const StaticIpDescr& s)
    {
        stream << versionForSerialization_;
        stream << s.id << s.ipId << s.staticIp << s.type << s.name << s.countryCode << s.shortName << s.cityName << s.serverId << s.nodeIPs
               << s.hostname << s.dnsHostname << s.username << s.password << s.wgIp << s.wgPubKey << s.ovpnX509 << s.ports;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, StaticIpDescr& s)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        stream >> s.id >> s.ipId >> s.staticIp >> s.type >> s.name >> s.countryCode >> s.shortName >> s.cityName >> s.serverId >> s.nodeIPs
               >> s.hostname >> s.dnsHostname >> s.username >> s.password >> s.wgIp >> s.wgPubKey >> s.ovpnX509 >> s.ports;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;
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

    friend QDataStream& operator <<(QDataStream& stream, const StaticIps& s)
    {
        stream << versionForSerialization_;
        stream << s.d->deviceName_ << s.d->ips_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, StaticIps& s)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        stream >> s.d->deviceName_ >> s.d->ips_;
        return stream;
    }

private:
    QSharedDataPointer<StaticIpsData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_STATICIPS_H
