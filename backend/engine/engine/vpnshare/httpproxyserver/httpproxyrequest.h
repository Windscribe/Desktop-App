#ifndef HTTPPROXYREQUEST_H
#define HTTPPROXYREQUEST_H

#include <string>
#include <QObject>
#include <QVector>
#include "httpproxyheader.h"

namespace HttpProxyServer {

class HttpProxyRequest
{
public:
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

    bool isConnectMethod();
    std::string getEstablishHttpConnectionMessage();
    long getContentLength();
    std::string processClientHeaders();

private:
    const int HTTP_PORT = 80;
    const int HTTP_PORT_SSL = 443;

    int extractUrl (const char *url, int default_port);
    void stripUsernamePassword (std::string &hostStr);
    int stripReturnPort(std::string &hostStr);

    bool shouldSkipHeader(const std::string &headerName);
};

} // namespace HttpProxyServer

#endif // HTTPPROXYREQUEST_H
