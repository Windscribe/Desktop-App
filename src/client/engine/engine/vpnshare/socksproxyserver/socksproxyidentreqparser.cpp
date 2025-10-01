#include "socksproxyidentreqparser.h"
#include "utils/ws_assert.h"

namespace SocksProxyServer {

SocksProxyIdentReqParser::SocksProxyIdentReqParser() : state_(version_number_of_methods),
    bytesReaded_(0)
{
    memset(&ident_req_, 0, sizeof(ident_req_));
}

bool SocksProxyIdentReqParser::parse(const QByteArray &arr, quint32 &outParsed)
{
    const char *data = arr.data();
    for (int i = 0; i < arr.size(); ++i)
    {
        if (consume(data[i]))
        {
            outParsed = i + 1;
            return true;
        }
    }

    outParsed = arr.size();
    return false;
}

bool SocksProxyIdentReqParser::consume(char input)
{
    if (state_ == version_number_of_methods)
    {
        if (bytesReaded_ < (sizeof(ident_req_.Version) + sizeof(identReq().NumberOfMethods)))
        {
            char *p = (char *)&ident_req_;
            p[bytesReaded_] = input;
            bytesReaded_++;
        }

        if (bytesReaded_ == (sizeof(ident_req_.Version) + sizeof(identReq().NumberOfMethods)))
        {
            state_ = methods;
        }
    }
    else if (state_ == methods)
    {
        if (bytesReaded_ < (sizeof(ident_req_.Version) + sizeof(identReq().NumberOfMethods)
                            + identReq().NumberOfMethods * sizeof(identReq().Methods[0])))
        {
            char *p = (char *)&ident_req_;
            p[bytesReaded_] = input;
            bytesReaded_++;
        }
        if (bytesReaded_ == (sizeof(ident_req_.Version) + sizeof(identReq().NumberOfMethods)
                            + identReq().NumberOfMethods * sizeof(identReq().Methods[0])))
        {
            return true;
        }
    }
    else
    {
        WS_ASSERT(false);
    }
    return false;
}

}
