#include "dnswhileconnectedtype.h"

#include <QObject>

DNSWhileConnectedType::DNSWhileConnectedType() : type_(ROBERT)
{

}

DNSWhileConnectedType::DNSWhileConnectedType(DNSWhileConnectedType::DNS_WHILE_CONNECTED_TYPE type) : type_(type)
{

}

DNSWhileConnectedType::DNSWhileConnectedType(int type)
{
    if (type >= ROBERT && type <= CUSTOM)
    {
        type_ = static_cast<DNS_WHILE_CONNECTED_TYPE>(type);
    }
    else
    {
        Q_ASSERT(false);
    }
}

QString DNSWhileConnectedType::toString() const
{
    if (type_ == ROBERT)
    {
        return QObject::tr("R.O.B.E.R.T.");
    }
    else if (type_ == CUSTOM)
    {
        return QObject::tr("Custom");
    }
    else
    {
        Q_ASSERT(false);
        return "";
    }
}

DNSWhileConnectedType::DNS_WHILE_CONNECTED_TYPE DNSWhileConnectedType::type() const
{
    return type_;
}

QList<DNSWhileConnectedType> DNSWhileConnectedType::allAvailableTypes()
{
    QList<DNSWhileConnectedType> t;
    t << DNSWhileConnectedType(ROBERT) << DNSWhileConnectedType(CUSTOM);
    return t;
}
