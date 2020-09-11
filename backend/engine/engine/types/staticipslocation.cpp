#include "staticipslocation.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include "../apiinfo/serverlocation.h"
#include "locationid.h"

StaticIpsLocation::StaticIpsLocation()
{

}

bool StaticIpsLocation::initFromJson(QJsonObject &init_obj)
{
    if (!init_obj.contains("static_ips"))
    {
        return false;
    }

    bool bDeviceNameFetched = false;

    QJsonArray jsonStaticIps = init_obj["static_ips"].toArray();
    Q_FOREACH(const QJsonValue &value, jsonStaticIps)
    {
        StaticIpDescr sid;
        QJsonObject obj = value.toObject();

        if (obj.contains("id") && obj.contains("ip_id") && obj.contains("static_ip") && obj.contains("type") &&
            obj.contains("name") && obj.contains("country_code") && obj.contains("short_name") &&
            obj.contains("server_id") && obj.contains("node"))
        {
            sid.id = obj["id"].toDouble();
            sid.ipId = obj["ip_id"].toDouble();
            sid.staticIp = obj["static_ip"].toString();
            sid.type = obj["type"].toString();
            sid.name = obj["name"].toString();
            sid.countryCode = obj["country_code"].toString();
            sid.shortName = obj["short_name"].toString();
            sid.serverId = obj["server_id"].toDouble();

            QJsonObject objNode = obj["node"].toObject();
            if (objNode.contains("ip") && objNode.contains("ip2") && objNode.contains("ip3") &&
                objNode.contains("city_name") && objNode.contains("hostname") && objNode.contains("dns_hostname"))
            {
                sid.nodeIP1 = objNode["ip"].toString();
                sid.nodeIP2 = objNode["ip2"].toString();
                sid.nodeIP3 = objNode["ip3"].toString();
                sid.cityName = objNode["city_name"].toString();
                sid.hostname = objNode["hostname"].toString();
                sid.dnsHostname = objNode["dns_hostname"].toString();
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        if (obj.contains("ports"))
        {
            const QJsonArray jsonPorts = obj["ports"].toArray();
            for (const QJsonValue &valuePort: jsonPorts)
            {
                QJsonObject objPort = valuePort.toObject();
                if (objPort.contains("ext_port") && objPort.contains("int_port"))
                {
                    StaticIpPortDescr sipd;
                    sipd.extPort = objPort["ext_port"].toDouble();
                    sipd.intPort = objPort["int_port"].toDouble();
                    sid.ports << sipd;
                }
                else
                {
                    return false;
                }
            }
        }


        if (!obj.contains("credentials"))
        {
            return false;
        }

        QJsonObject objCredentials = obj["credentials"].toObject();
        if (objCredentials.contains("username") && objCredentials.contains("password"))
        {
            sid.username = objCredentials["username"].toString();
            sid.password = objCredentials["password"].toString();
        }
        else
        {
            return false;
        }

        if (!bDeviceNameFetched && obj.contains("device_name"))
        {
            deviceName_ = obj["device_name"].toString();
            bDeviceNameFetched = true;
        }

        ips_ << sid;
    }

    return true;
}

void StaticIpsLocation::writeToStream(QDataStream &stream) const
{
    stream << REVISION_VERSION;
    int cnt = ips_.count();
    stream << cnt;
    for (int i = 0; i < cnt; ++i)
    {
        stream << ips_[i].id;
        stream << ips_[i].ipId;
        stream << ips_[i].staticIp;
        stream << ips_[i].type;
        stream << ips_[i].name;
        stream << ips_[i].countryCode;
        stream << ips_[i].shortName;
        stream << ips_[i].cityName;
        stream << ips_[i].serverId;
        stream << ips_[i].nodeIP1;
        stream << ips_[i].nodeIP2;
        stream << ips_[i].nodeIP3;
        stream << ips_[i].hostname;
        stream << ips_[i].dnsHostname;

        int cntPorts = ips_[i].ports.count();
        stream << cntPorts;
        for (int k = 0; k < cntPorts; ++k)
        {
            stream << ips_[i].ports[k].extPort;
            stream << ips_[i].ports[k].intPort;
        }

        stream << ips_[i].username;
        stream << ips_[i].password;
    }
    stream << deviceName_;
}

void StaticIpsLocation::readFromStream(QDataStream &stream, int revision)
{
    ips_.clear();
    deviceName_.clear();

    int revisionVersion;
    stream >> revisionVersion;
    int cnt;
    stream >> cnt;
    for (int i = 0; i < cnt; ++i)
    {
        StaticIpDescr sid;

        stream >> sid.id;
        stream >> sid.ipId;
        stream >> sid.staticIp;
        stream >> sid.type;
        stream >> sid.name;
        stream >> sid.countryCode;
        stream >> sid.shortName;
        stream >> sid.cityName;
        stream >> sid.serverId;
        stream >> sid.nodeIP1;
        stream >> sid.nodeIP2;
        stream >> sid.nodeIP3;
        stream >> sid.hostname;
        stream >> sid.dnsHostname;

        int cntPorts;
        stream >> cntPorts;
        for (int k = 0; k < cntPorts; ++k)
        {
            StaticIpPortDescr sipd;
            stream >> sipd.extPort;
            stream >> sipd.intPort;
            sid.ports << sipd;
        }

        stream >> sid.username;
        stream >> sid.password;

        ips_ << sid;
    }

    if (revision >= 9)
    {
        stream >> deviceName_;
    }
}

QSharedPointer<ServerLocation> StaticIpsLocation::makeServerLocation()
{
    QSharedPointer<ServerLocation> sl(new ServerLocation);
    /*sl->id_ = LocationID::STATIC_IPS_LOCATION_ID;
    sl->type_ = ServerLocation::SERVER_LOCATION_STATIC;
    sl->name_ = QObject::tr("Static IPs");
    sl->countryCode_ = "STATIC_IPS";
    sl->premiumOnly_ = false;
    sl->p2p_ = 1;
    sl->dnsHostName_ = "";
    sl->staticIpsDeviceName_ = deviceName_;

    for (int i = 0; i < ips_.count(); ++i)
    {
        ServerNode sn;
        sn.initFromStaticIpDescr(ips_[i]);
        sl->nodes_ << sn;
    }

    sl->isValid_ = true;

    sl->makeInternalStates();*/

    return sl;
}

QString StaticIpPortsVector::getAsStringWithDelimiters() const
{
    QString ret;

    for (int i = 0; i < count(); ++i)
    {
        ret += QString::number(at(i));
        if (i < (count() - 1))
        {
            ret += ",";
        }
    }

    return ret;
}

bool operator==(const StaticIpPortDescr &l, const StaticIpPortDescr &r)
{
    return l.extPort == r.extPort && l.intPort == r.intPort;
}
