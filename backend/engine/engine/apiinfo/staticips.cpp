#include "staticips.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>

const int typeIdStaticIps = qRegisterMetaType<apiinfo::StaticIps>("apiinfo::StaticIps");

namespace apiinfo {

bool StaticIps::initFromJson(QJsonObject &init_obj)
{
    if (!init_obj.contains("static_ips"))
    {
        return false;
    }

    const QJsonArray jsonStaticIps = init_obj["static_ips"].toArray();
    for (const QJsonValue &value : jsonStaticIps)
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
            sid.countryCode = obj["country_code"].toString().toLower();
            sid.shortName = obj["short_name"].toString();
            sid.serverId = obj["server_id"].toDouble();
            sid.wgIp = obj["wg_ip"].toString();
            sid.wgPubKey = obj["wg_pubkey"].toString();
            sid.ovpnX509 = obj["ovpn_x509"].toString();

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

        if (obj.contains("device_name"))
        {
            d->deviceName_ = obj["device_name"].toString();
        }

        d->ips_ << sid;
    }

    return true;
}

void StaticIps::initFromProtoBuf(const ProtoApiInfo::StaticIps &staticIps)
{
    d->deviceName_ = QString::fromStdString(staticIps.device_name());
    d->ips_.clear();
    for (int i = 0; i < staticIps.ips_size(); ++i)
    {
        const ProtoApiInfo::StaticIpDescr &sd = staticIps.ips(i);
        StaticIpDescr sid;
        sid.id = sd.id();
        sid.ipId = sd.ip_id();
        sid.staticIp = QString::fromStdString(sd.static_ip());
        sid.type = QString::fromStdString(sd.type());
        sid.name = QString::fromStdString(sd.name());
        sid.countryCode = QString::fromStdString(sd.country_code());
        sid.shortName = QString::fromStdString(sd.short_name());
        sid.cityName = QString::fromStdString(sd.city_name());
        sid.serverId = sd.server_id();
        Q_ASSERT(sd.node_ips_size() == 3);
        if (sd.node_ips_size() == 3)
        {
            sid.nodeIP1 = QString::fromStdString(sd.node_ips(0));
            sid.nodeIP2 = QString::fromStdString(sd.node_ips(1));
            sid.nodeIP3 = QString::fromStdString(sd.node_ips(2));
        }
        sid.hostname = QString::fromStdString(sd.hostname());
        sid.dnsHostname = QString::fromStdString(sd.dns_hostname());
        sid.username = QString::fromStdString(sd.username());
        sid.password = QString::fromStdString(sd.password());
        sid.wgIp = QString::fromStdString(sd.wg_ip());
        sid.wgPubKey = QString::fromStdString(sd.wg_pubkey());
        sid.ovpnX509 = QString::fromStdString(sd.ovpn_x509());
        for (int p = 0; p < sd.ports_size(); ++p)
        {
            StaticIpPortDescr portDescr;
            portDescr.extPort = sd.ports(p).ext_port();
            portDescr.intPort = sd.ports(p).int_port();
            sid.ports << portDescr;
        }
        d->ips_ << sid;
    }
}

ProtoApiInfo::StaticIps StaticIps::getProtoBuf() const
{
    ProtoApiInfo::StaticIps si;
    si.set_device_name(d->deviceName_.toStdString());
    for (const StaticIpDescr &sid : d->ips_)
    {
        ProtoApiInfo::StaticIpDescr *pi = si.add_ips();
        pi->set_id(sid.id);
        pi->set_ip_id(sid.ipId);
        pi->set_static_ip(sid.staticIp.toStdString());
        pi->set_type(sid.type.toStdString());
        pi->set_name(sid.name.toStdString());
        pi->set_country_code(sid.countryCode.toStdString());
        pi->set_short_name(sid.shortName.toStdString());
        pi->set_city_name(sid.cityName.toStdString());
        pi->set_server_id(sid.serverId);
        *pi->add_node_ips() = sid.nodeIP1.toStdString();
        *pi->add_node_ips() = sid.nodeIP2.toStdString();
        *pi->add_node_ips() = sid.nodeIP3.toStdString();
        pi->set_hostname(sid.hostname.toStdString());
        pi->set_dns_hostname(sid.dnsHostname.toStdString());
        pi->set_username(sid.username.toStdString());
        pi->set_password(sid.password.toStdString());
        pi->set_wg_ip(sid.wgIp.toStdString());
        pi->set_wg_pubkey(sid.wgPubKey.toStdString());
        pi->set_ovpn_x509(sid.ovpnX509.toStdString());

        for (const StaticIpPortDescr &portDescr : sid.ports)
        {
            ProtoApiInfo::StaticIpPortDescr *port = pi->add_ports();
            port->set_ext_port(portDescr.extPort);
            port->set_int_port(portDescr.intPort);
        }
    }
    return si;
}

QStringList StaticIps::getAllPingIps() const
{
    QStringList ret;
    for (const StaticIpDescr &sid : d->ips_)
    {
        ret << sid.getPingIp();
    }
    return ret;
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

} //namespace apiinfo
