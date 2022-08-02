#include "dnswhileconnectedinfo.h"
#include <QObject>

namespace types {

DnsWhileConnectedInfo::DnsWhileConnectedInfo(ProtoTypes::DnsWhileConnectedInfo protoBufInfo) :
    type_(static_cast<DNS_WHILE_CONNECTED_TYPE>(protoBufInfo.type()))
  , ipAddress_(QString::fromStdString(protoBufInfo.ip_address()))
{

}

ProtoTypes::DnsWhileConnectedInfo DnsWhileConnectedInfo::toProtobuf()
{
    ProtoTypes::DnsWhileConnectedInfo pDns;
    pDns.set_type(static_cast<ProtoTypes::DnsWhileConnectedType>(type_));
    pDns.set_ip_address(ipAddress_.toStdString());
    return pDns;
}

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


} // types namespace
