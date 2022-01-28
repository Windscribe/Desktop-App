#ifndef SOCKSPROXYIDENTREQPARSER_H
#define SOCKSPROXYIDENTREQPARSER_H

#include <QByteArray>
#include "socksstructs.h"

namespace SocksProxyServer {

class SocksProxyIdentReqParser
{
public:
    SocksProxyIdentReqParser();

    bool parse(const QByteArray &arr, quint32 &outParsed);
    const socks5_ident_req &identReq() { return ident_req_; }

private:
    socks5_ident_req ident_req_;

    bool consume(char input);

    enum state
    {
        version_number_of_methods,
        methods
    } state_;

    unsigned int bytesReaded_;
};

}

#endif // SOCKSPROXYIDENTREQPARSER_H
