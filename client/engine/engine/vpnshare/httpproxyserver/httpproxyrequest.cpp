#include "httpproxyrequest.h"

#include "utils/ws_assert.h"
#include "utils/log/logger.h"
#include "utils/boost_includes.h"
#include "version/appversion.h"

#include <stdlib.h>

#ifdef Q_OS_WIN
    #include <ws2tcpip.h>
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace HttpProxyServer {

void HttpProxyRequest::debugToLog()
{
    QString str;
    str = "Method: " + QString::fromStdString(method) + "; URI: " + QString::fromStdString(uri);
    str += "; Headers(";
    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        str += QString::fromStdString(it->name) + "=" + QString::fromStdString(it->value);
        if (it != (headers.end() - 1))
        {
            str += ";";
        }
    }
    str += ")";

    qCDebug(LOG_HTTP_SERVER) << "Request:" << str;
}

#ifdef Q_OS_WIN
int strncasecmp(const char *s1, const char *s2, size_t count)
{
    return _strnicmp(s1, s2, count);
}
int inet_pton(int af, const char *src, void *dst)
{
  struct sockaddr_storage ss;
  int size = sizeof(ss);
  char src_copy[INET6_ADDRSTRLEN+1];

  ZeroMemory(&ss, sizeof(ss));
  /* stupid non-const API */
  strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
  src_copy[INET6_ADDRSTRLEN] = 0;

  if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
    switch(af) {
      case AF_INET:
    *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
    return 1;
      case AF_INET6:
    *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
    return 1;
    }
  }
  return 0;
}
#endif

bool HttpProxyRequest::extractHostAndPort()
{
    if (strncasecmp (uri.c_str(), "http://", 7) == 0)
    {
        char *skipped_type = strstr((char *)uri.c_str(), "//") + 2;
        if (extractUrl(skipped_type, HTTP_PORT) < 0)
        {
            return false;
        }
    }
    else if (strcmp (method.c_str(), "CONNECT") == 0)
    {
        if (extractUrl (uri.c_str(), HTTP_PORT_SSL) < 0)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool HttpProxyRequest::isConnectMethod() const
{
    return strcmp (method.c_str(), "CONNECT") == 0;
}

std::string HttpProxyRequest::getEstablishHttpConnectionMessage()
{
    std::string msg;
    char portbuff[7];
    char dst[sizeof(struct in6_addr)];

    // Build a port string if it's not a standard port
    if (port != HTTP_PORT && port != HTTP_PORT_SSL)
    {
#ifdef Q_OS_WIN
        _snprintf (portbuff, 7, ":%u", port);
#else
        snprintf (portbuff, 7, ":%u", port);
#endif
    }
    else
    {
        portbuff[0] = '\0';
    }

    if (inet_pton(AF_INET6, host.c_str(), dst) > 0)
    {
        // host is an IPv6 address literal, so surround it with []
        msg = method + " " + path + " HTTP/1.0\r\n";
        msg += "Host: [" + host + "]" + portbuff + "\r\n";
        msg += "Connection: close\r\n";

        /*return write_message (connptr->server_fd,
                                          "%s %s HTTP/1.0\r\n"
                                          "Host: [%s]%s\r\n"
                                          "Connection: close\r\n",
                                          request->method, request->path,
                                          request->host, portbuff);*/
    }
    else
    {
        msg = method + " " + path + " HTTP/1.0\r\n";
        msg += "Host: " + host + portbuff + "\r\n";
        msg += "Connection: close\r\n";

        /*return write_message (connptr->server_fd,
                                          "%s %s HTTP/1.0\r\n"
                                          "Host: %s%s\r\n"
                                          "Connection: close\r\n",
                                          request->method, request->path,
                                          request->host, portbuff);*/
    }
    return msg;
}

long HttpProxyRequest::getContentLength()
{
    //todo: make map instead vector
    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (boost::iequals(it->name,"content-length"))
        {
            return atol (it->value.c_str());
        }
    }
    return -1;
}

std::string HttpProxyRequest::processClientHeaders()
{
    std::string ret;
    //todo: check buffer bounds
    char buf[4096];
    bool isExistViaHeader = false;
    assert(!isConnectMethod());

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        if (!shouldSkipHeader(it->name))
        {
            if (boost::iequals(it->name,"via"))
            {
                int len = snprintf(buf, 4096, "Via: %s, %hu.%hu %s (%s/%s)\r\n",
                        it->value.c_str(), http_version_major, http_version_minor, "Windscribe proxy", AppVersion::instance().version().toStdString().c_str(),
                        AppVersion::instance().build().toStdString().c_str());
                WS_ASSERT((unsigned int)len < sizeof(buf));
                ret += buf;
                isExistViaHeader = true;
            }
            else
            {
                ret += it->name + ": " + it->value + "\r\n";
            }
        }
    }

    if (!isExistViaHeader)
    {
        int len = snprintf(buf, 4096, "Via: %hu.%hu %s (%s/%s)\r\n",
                http_version_major, http_version_minor, "Windscribe proxy", AppVersion::instance().version().toStdString().c_str(),
                AppVersion::instance().build().toStdString().c_str());
        WS_ASSERT((unsigned int)len < sizeof(buf));
        ret += buf;
    }

    ret += "\r\n";

    return ret;
}

int HttpProxyRequest::extractUrl(const char *url, int default_port)
{
    char *p;

    // Split the URL on the slash to separate host from path
    p = strchr ((char *)url, '/');
    if (p != NULL)
    {
        int len;
        len = p - url;
        host = std::string(url, url + len);
        path = std::string(p);
    }
    else
    {
        host = std::string(url);
        path = "/";
    }


    if (host.empty() || path.empty())
    {
        return -1;
    }

    // Remove the username/password if they're present
    stripUsernamePassword (host);

    // Find a proper port in www.site.com:8001 URLs
    int portRet = stripReturnPort(host);

    port = (portRet != 0) ? portRet : default_port;

    // Remove any surrounding '[' and ']' from IPv6 literals
    host.insert(host.begin(), '[');
    host.insert(host.end(), ']');
    p = strrchr ((char *)host.c_str(), ']');
    if (p && (*(host.c_str()) == '['))
    {
        host.erase(host.begin());
        host.erase(host.end() - 1);
    }

    return 0;
}

void HttpProxyRequest::stripUsernamePassword (std::string &hostStr)
{
    char *p;
    WS_ASSERT(hostStr.length() > 0);

    if ((p = strchr ((char *)hostStr.c_str(), '@')) == NULL)
        return;

    char *temp = new char[hostStr.length()];
    char *offs = temp;
    // Move the pointer past the "@" and then copy from that point until the NULL to the beginning of the host buffer.
    p++;
    while (*p)
    {
        *offs++ = *p++;
    }
    *offs = '\0';

    hostStr = std::string(temp);
    delete[] temp;
}

int HttpProxyRequest::stripReturnPort(std::string &hostStr)
{
    char *ptr1;
    char *ptr2;
    int return_port;

    ptr1 = strrchr ((char *)hostStr.c_str(), ':');
    if (ptr1 == NULL)
    {
        return 0;
    }

    // Check for IPv6 style literals
    ptr2 = strchr (ptr1, ']');
    if (ptr2 != NULL)
    {
        return 0;
    }

    ptr1++;
    if (sscanf (ptr1, "%d", &return_port) != 1)    // one conversion required
    {
        return_port = 0;
    }

    hostStr.erase(hostStr.rfind(':'));

    return return_port;
}

bool HttpProxyRequest::shouldSkipHeader(const std::string &headerName)
{
    if (boost::iequals(headerName,"connection"))
        return true;
    if (boost::iequals(headerName,"proxy-connection"))
        return true;
    if (boost::iequals(headerName,"host"))
        return true;
    if (boost::iequals(headerName,"keep-alive"))
        return true;
    if (boost::iequals(headerName,"te"))
        return true;
    if (boost::iequals(headerName,"trailers"))
        return true;
    if (boost::iequals(headerName,"upgrade"))
        return true;

    return false;
}

} // namespace HttpProxyServer
