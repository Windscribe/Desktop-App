#include "detectlanrange.h"
#include "VpnShare/SocketUtils/detectlocalip.h"
//#include <winsock2.h>
//#include <iphlpapi.h>
#ifdef Q_OS_WIN
    #include <Ws2tcpip.h>
#else
    #include <arpa/inet.h>
#endif


DetectLanRange::DetectLanRange()
{

}

bool DetectLanRange::isRfcRangeAddress(quint32 ip)
{
    // 192.168.0.0 - 192.168.255.255
    if ((ip & 0x0000FFFF) == 0xA8C0)
    {
        return true;
    }
    // 172.16.0.0 - 172.31.255.255
    if ((ip & 0x0000F0FF) == 0x10AC)
    {
        return true;
    }
    // 10.0.0.0 - 10.255.255.255
    if ((ip & 0xFF) == 0x0A)
    {
        return true;
    }
    // 224.0.0.0 - 239.255.255.255
    if ((ip & 0x70) == 0x60)
    {
        return true;
    }
    return false;
}

bool DetectLanRange::isRfcLanRange()
{
    quint32 ip;
    QString strIp = DetectLocalIP::getLocalIP();
#ifdef Q_OS_WIN
    if (InetPtonA(AF_INET, strIp.toStdString().c_str(), &ip) == 1)
#else
    if (inet_pton(AF_INET, strIp.toStdString().c_str(), &ip) == 1)
#endif
    {
        ips_.clear();
        ips_ << strIp;
        return isRfcRangeAddress(ip);
    }
    else
    {
        return true;
    }

    /*
    bIsRfcRange = true;
    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    ULONG iterations = 0;
    const int MAX_TRIES = 5;
    QByteArray arr;

    outBufLen  = arr.size();
    do {
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST |
                                        GAA_FLAG_SKIP_MULTICAST |
                                        GAA_FLAG_SKIP_DNS_SERVER |
                                        GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, (PIP_ADAPTER_ADDRESSES)arr.data(), &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            arr.resize(outBufLen);
        }
        else
        {
            break;
        }
        iterations++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iterations < MAX_TRIES));

    if (dwRetVal != NO_ERROR)
    {
        return bIsRfcRange;
    }

    PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)arr.data();
    while (pCurrAddresses)
    {
        // Skip loopback adapters
        if (pCurrAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK || pCurrAddresses->OperStatus != IfOperStatusUp)
        {
            pCurrAddresses = pCurrAddresses->Next;
            continue;
        }

        for (IP_ADAPTER_UNICAST_ADDRESS* address = pCurrAddresses->FirstUnicastAddress;
              NULL != address;
              address = address->Next)
        {
            auto family = address->Address.lpSockaddr->sa_family;
            if (AF_INET == family) // IPv4
            {

                SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);

                char strIp[16];
                InetNtopA(AF_INET, &ipv4->sin_addr, strIp, sizeof(strIp));
                ips_ << QString::fromStdString(strIp);

                quint32 ip;
                memcpy(&ip, &(ipv4->sin_addr), sizeof(ipv4->sin_addr));
                if (!isRfcRangeAddress(ip))
                {
                    bIsRfcRange = false;
                    break;
                }
            }
        }
        pCurrAddresses = pCurrAddresses->Next;
    }
    return bIsRfcRange;*/
}

QStringList DetectLanRange::getIps()
{
    return ips_;
}
