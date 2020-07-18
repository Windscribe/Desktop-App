#include "socksproxycommandparser.h"

namespace SocksProxyServer {

SocksProxyCommandParser::SocksProxyCommandParser()
{
    reset();

}

void SocksProxyCommandParser::reset()
{
    state_ = first4bytes;
    bytesReaded_ = 0;
}

TRI_BOOL SocksProxyCommandParser::parse(const QByteArray &arr, quint32 &outParsed)
{
    const char *data = arr.data();
    for (int i = 0; i < arr.size(); ++i)
    {
        TRI_BOOL res = consume(data[i]);
        if (res == TRI_TRUE || res == TRI_FALSE)
        {
            outParsed = i + 1;
            return res;
        }
    }

    outParsed = arr.size();
    return TRI_INDETERMINATE;
}

TRI_BOOL SocksProxyCommandParser::consume(char input)
{
    if (state_ == first4bytes)
    {
        unsigned int len = sizeof(cmd_.Version) + sizeof(cmd_.Cmd) + sizeof(cmd_.Reserved) + sizeof(cmd_.AddrType);
        if (bytesReaded_ < len)
        {
            char *p = (char *)&cmd_;
            p[bytesReaded_] = input;
            bytesReaded_++;
        }

        if (bytesReaded_ == len)
        {
            if (cmd_.AddrType == 0x01)
            {
                addressReaded_ = 0;
                state_ = address_ipv4;
            }
            else if (cmd_.AddrType == 0x04)
            {
                addressReaded_ = 0;
                state_ = address_ipv6;
            }
            else if (cmd_.AddrType == 0x03)
            {
                state_ = domain_len;
            }
            else
            {
                return TRI_FALSE;
            }
        }
    }
    else if (state_ == domain_len)
    {
        cmd_.DestAddr.DomainLen = input;
        bytesReaded_++;
        domainNameReaded_ = 0;
        state_ = domain_name;
    }
    else if (state_ == domain_name)
    {
        cmd_.DestAddr.Domain[domainNameReaded_] = input;
        domainNameReaded_++;
        if (domainNameReaded_ == cmd_.DestAddr.DomainLen)
        {
            portReaded_ = 0;
            state_ = port;
        }
    }
    else if (state_ == address_ipv4)
    {
        char *p = (char *)&cmd_.DestAddr.IPv4;
        p[sizeof(cmd_.DestAddr.IPv4) - addressReaded_ - 1] = input;
        addressReaded_++;
        if (addressReaded_ == sizeof(cmd_.DestAddr.IPv4))
        {
            portReaded_ = 0;
            state_ = port;
        }
    }
    else if (state_ == address_ipv6)
    {
        char *p = (char *)&cmd_.DestAddr.IPv6;
        p[sizeof(cmd_.DestAddr.IPv6) - addressReaded_ - 1] = input;
        addressReaded_++;
        if (addressReaded_ == sizeof(cmd_.DestAddr.IPv6))
        {
            portReaded_ = 0;
            state_ = port;
        }
    }
    else if (state_ == port)
    {
        char *p = (char *)&cmd_.DestPort;
        p[sizeof(cmd_.DestPort) - portReaded_ - 1] = input;
        portReaded_++;
        if (portReaded_ == sizeof(cmd_.DestPort))
        {
            return TRI_TRUE;
        }
    }
    else
    {
        Q_ASSERT(false);
    }
    return TRI_INDETERMINATE;
}

}
