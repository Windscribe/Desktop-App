#ifndef DNSWHILECONNECTEDTYPE_H
#define DNSWHILECONNECTEDTYPE_H

#include <QString>

class DNSWhileConnectedType
{
public:
    enum DNS_WHILE_CONNECTED_TYPE { ROBERT, CUSTOM };

    DNSWhileConnectedType();
    explicit DNSWhileConnectedType(DNS_WHILE_CONNECTED_TYPE type);
    explicit DNSWhileConnectedType(int type);

    QString toString() const;
    DNS_WHILE_CONNECTED_TYPE type() const;

    static QList<DNSWhileConnectedType> allAvailableTypes();

private:
    DNS_WHILE_CONNECTED_TYPE type_;

};

inline bool operator==(const DNSWhileConnectedType& lhs, const DNSWhileConnectedType& rhs)
{
    return lhs.type() == rhs.type();
}

inline bool operator!=(const DNSWhileConnectedType& lhs, const DNSWhileConnectedType& rhs)
{
    return lhs.type() != rhs.type();
}

#endif // DNSWHILECONNECTEDTYPE_H
