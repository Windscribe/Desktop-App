#pragma once

#ifdef Q_OS_WIN
    #include <Winsock2.h>
    #include <Ws2tcpip.h>
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    #include <arpa/inet.h>
#endif

namespace SocksProxyServer {

#ifdef Q_OS_WIN
#pragma pack(push, 1)
#endif

struct socks5_ident_req
{
    unsigned char Version;
    unsigned char NumberOfMethods;
    unsigned char Methods[256];
}
#ifdef Q_OS_WIN
;
#endif
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
__attribute__((aligned(1)));
#endif

struct socks5_answer
{
    unsigned char Version;
    unsigned char Method;
}
#ifdef Q_OS_WIN
;
#endif
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
__attribute__((aligned(1)));
#endif

struct socks5_req
{
    unsigned char Version;
    unsigned char Cmd;
    unsigned char Reserved;
    unsigned char AddrType;
    union {
        in_addr IPv4;
        in6_addr IPv6;
        struct {
            unsigned char DomainLen;
            char Domain[256];
        };
    } DestAddr;
    unsigned short DestPort;
}
#ifdef Q_OS_WIN
;
#endif
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
__attribute__((aligned(1)));
#endif

struct socks5_resp
{
    unsigned char Version;
    unsigned char Reply;
    unsigned char Reserved;
    unsigned char AddrType;
    union {
        in_addr IPv4;
        in6_addr IPv6;
        struct {
            unsigned char DomainLen;
            char Domain[256];
        };
    } BindAddr;
    unsigned short BindPort;
}
#ifdef Q_OS_WIN
;
#endif
#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
__attribute__((aligned(1)));
#endif

#ifdef Q_OS_WIN
#pragma pack(pop)
#endif

} // namespace SocksProxyServer
