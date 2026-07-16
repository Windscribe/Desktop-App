#include "availableport.h"
#include <QObject>

#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    #include <netdb.h>
    #include <unistd.h>
#endif

unsigned int AvailablePort::getAvailablePort(unsigned int defaultPort)
{
#ifdef Q_OS_WIN
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo("127.0.0.1", 0, &hints, &result);
    if ( iResult != 0 )
    {
        return defaultPort;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        return defaultPort;
    }

    iResult = bind( sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        freeaddrinfo(result);
        closesocket(sock);
        return defaultPort;
    }
    struct sockaddr_in foo;
    int len = sizeof(struct sockaddr);
    int retPort = -1;
    if (getsockname(sock, (struct sockaddr *) &foo, &len) != SOCKET_ERROR)
    {
        retPort = ntohs(foo.sin_port);
    }

    freeaddrinfo(result);
    closesocket(sock);
    return retPort;
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if  (sock < 0)
    {
        return defaultPort;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0;
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return defaultPort;
    }

    socklen_t len = sizeof(serv_addr);
    if (getsockname(sock, (struct sockaddr *)&serv_addr, &len) == -1)
    {
        close(sock);
        return defaultPort;
    }

    unsigned int retPort = ntohs(serv_addr.sin_port);
    close (sock);
    return retPort;
#endif
}

bool AvailablePort::isPortBusy(const QString &ip, unsigned int port)
{
#ifdef Q_OS_WIN
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult = getaddrinfo(ip.toStdString().c_str(), QString::number(port).toStdString().c_str(), &hints, &result);
    if ( iResult != 0 )
        return true;

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        return true;
    }

    iResult = bind( sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        freeaddrinfo(result);
        closesocket(sock);
        return true;
    }

    freeaddrinfo(result);
    closesocket(sock);
    return false;

#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    // In most UNIX-like environments, this may fail on all addresses if port is <1024 since this process is not root
    // so this is a meaningless test.
    return true;
#endif
}
