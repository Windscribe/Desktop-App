#include "dnswhileconnectedinfo.h"

#include <QObject>

DnsWhileConnectedInfo::DnsWhileConnectedInfo() : type_(ROBERT), ipAddress_("")
{

}

DnsWhileConnectedInfo::DnsWhileConnectedInfo(DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE type) :
    type_(type)
  , ipAddress_("")
{

}

DnsWhileConnectedInfo::DnsWhileConnectedInfo(ProtoTypes::DnsWhileConnectedInfo protoBufInfo) :
    type_(static_cast<DNS_WHILE_CONNECTED_TYPE>(protoBufInfo.type()))
  , ipAddress_(QString::fromStdString(protoBufInfo.ip_address()))
{

}

//DnsWhileConnectedInfo::DnsWhileConnectedInfo(int type)
//{
//    if (type >= ROBERT && type <= CUSTOM)
//    {
//        type_ = static_cast<DNS_WHILE_CONNECTED_TYPE>(type);
//    }
//    else
//    {
//        Q_ASSERT(false);
//    }
//}

QString DnsWhileConnectedInfo::toString() const
{
    return typeToString(type_);
}

DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE DnsWhileConnectedInfo::type() const
{
    return type_;
}

void DnsWhileConnectedInfo::setType(DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE type)
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

QList<DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE> DnsWhileConnectedInfo::allAvailableTypes()
{
    QList<DNS_WHILE_CONNECTED_TYPE> t;
    t << ROBERT << CUSTOM;
    return t;
}

QString DnsWhileConnectedInfo::typeToString(const DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE &type)
{
    if (type == ROBERT)
    {
        return robertText();
    }
    else if (type == CUSTOM)
    {
        return customText();
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

QString DnsWhileConnectedInfo::robertText()
{
    return QObject::tr("R.O.B.E.R.T.");
}

QString DnsWhileConnectedInfo::customText()
{
    return QObject::tr("Custom");
}

ProtoTypes::DnsWhileConnectedInfo DnsWhileConnectedInfo::toProtobuf()
{
    ProtoTypes::DnsWhileConnectedInfo pDns;
    pDns.set_type(static_cast<ProtoTypes::DnsWhileConnectedType>(type_));
    pDns.set_ip_address(ipAddress_.toStdString());
    return pDns;
}
