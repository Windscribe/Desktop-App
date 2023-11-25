#pragma once

#include <QByteArray>
#include <vector>
#include "httpproxyheader.h"

namespace HttpProxyServer {

class HttpProxyReply
{
public:
    // The status of the reply.
    enum status_type
    {
       ok = 200,
       created = 201,
       accepted = 202,
       no_content = 204,
       multiple_choices = 300,
       moved_permanently = 301,
       moved_temporarily = 302,
       not_modified = 304,
       bad_request = 400,
       unauthorized = 401,
       forbidden = 403,
       not_found = 404,
       internal_server_error = 500,
       not_implemented = 501,
       bad_gateway = 502,
       service_unavailable = 503
    } status;

    // The headers to be included in the reply.
    std::vector<HttpProxyHeader> headers;

    // The content to be sent in the reply.
    std::string content;

    QByteArray toBuffer();

    // Get a stock reply.
    static HttpProxyReply stock_reply(status_type status);
};

} // namespace HttpProxyServer
