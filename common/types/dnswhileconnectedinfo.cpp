#include "dnswhileconnectedinfo.h"
#include <QObject>

namespace types {

QString DnsWhileConnectedInfo::toString() const
{
    return typeToString(type_);
}

DNS_WHILE_CONNECTED_TYPE DnsWhileConnectedInfo::type() const
{
    return type_;
}

void DnsWhileConnectedInfo::setType(DNS_WHILE_CONNECTED_TYPE type)
{
    type_ = type;
}

QString DnsWhileConnectedInfo::ipAddress() const
{
    return ipAddress_;
}

void DnsWhileConnectedInfo::setIpAddress(const QString &ipAddr)
{
    ipAddress_ = ipAddr;
}

QList<DNS_WHILE_CONNECTED_TYPE> DnsWhileConnectedInfo::allAvailableTypes()
{
    QList<DNS_WHILE_CONNECTED_TYPE> t;
    t << DNS_WHILE_CONNECTED_TYPE_ROBERT << DNS_WHILE_CONNECTED_TYPE_CUSTOM;
    return t;
}

QString DnsWhileConnectedInfo::typeToString(const DNS_WHILE_CONNECTED_TYPE &type)
{
    if (type == DNS_WHILE_CONNECTED_TYPE_ROBERT)
    {
        return QObject::tr("R.O.B.E.R.T.");
    }
    else if (type == DNS_WHILE_CONNECTED_TYPE_CUSTOM)
    {
        return QObject::tr("Custom");
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

QJsonObject DnsWhileConnectedInfo::toJsonObject() const
{
    QJsonObject json;
    json["type"] = (int)type_;
    json["ipAddress"] = ipAddress_;
    return json;
}

bool DnsWhileConnectedInfo::fromJsonObject(const QJsonObject &json)
{
    if (json.contains("type")) type_ = (DNS_WHILE_CONNECTED_TYPE)json["type"].toInt(DNS_WHILE_CONNECTED_TYPE_ROBERT);
    if (json.contains("ipAddress")) ipAddress_ = json["ipAddress"].toString();
    return true;
}


} // types namespace
