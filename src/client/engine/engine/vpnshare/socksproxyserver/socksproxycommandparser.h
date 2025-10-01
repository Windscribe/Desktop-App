#pragma once

#include <QByteArray>
#include "socksstructs.h"

namespace SocksProxyServer {

enum TRI_BOOL { TRI_FALSE, TRI_TRUE, TRI_INDETERMINATE };

class SocksProxyCommandParser
{
public:
    SocksProxyCommandParser();

    void reset();
    TRI_BOOL parse(const QByteArray &arr, quint32 &outParsed);

    const socks5_req &cmd() const { return cmd_; }

private:
    socks5_req cmd_;

    TRI_BOOL consume(char input);

    enum state
    {
        first4bytes,
        domain_len,
        domain_name,
        address_ipv4,
        address_ipv6,
        port
    } state_;

    unsigned int bytesReaded_;
    unsigned char domainNameReaded_;
    unsigned char portReaded_;
    unsigned char addressReaded_;
};

}
