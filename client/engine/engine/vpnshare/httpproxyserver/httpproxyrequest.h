#pragma once

#include <string>
#include <QObject>
#include <QVector>
#include "httpproxyheader.h"

namespace HttpProxyServer {

class HttpProxyRequest
{
public:
    HttpProxyRequest() : http_version_major(0), http_version_minor(0), port(0) {}

    std::string method;
    std::string uri;
    unsigned short http_version_major;
    unsigned short http_version_minor;
    QVector< HttpProxyHeader > headers;

    std::string host;
    quint16 port;
    std::string path;

    void debugToLog();
    bool extractHostAndPort();

    bool isConnectMethod() const;
    std::string getEstablishHttpConnectionMessage();
    long getContentLength();
    std::string processClientHeaders();

private:
    static constexpr int HTTP_PORT = 80;
    static constexpr int HTTP_PORT_SSL = 443;

    int extractUrl (const char *url, int default_port);
    void stripUsernamePassword (std::string &hostStr);
    int stripReturnPort(std::string &hostStr);

    bool shouldSkipHeader(const std::string &headerName);
};

} // namespace HttpProxyServer
