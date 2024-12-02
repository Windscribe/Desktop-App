#include "detectlanrange.h"
#include "utils/log/categories.h"

// static
bool DetectLanRange::isRfcLoopbackAddress(quint32 ip)
{
    // 127.0.0.0 - 127.255.255.255
    if ((ip & 0xFF) == 0x7F)
    {
        return true;
    }
    return false;
}

// static
bool DetectLanRange::isRfcPrivateAddress(quint32 ip)
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
    // 169.254.0.0 - 169.254.255.255
    if ((ip & 0x0000FFFF) == 0xFEA9)
    {
        return true;
    }
    // 10.0.0.0 - 10.255.255.255
    if ((ip & 0xFF) == 0x0A)
    {
        return true;
    }
    // 224.0.0.0 - 239.255.255.255
    if ((ip & 0xF0) == 0xE0)
    {
        return true;
    }
    return false;
}

// static
bool DetectLanRange::isRfcLanRange(const QString &address)
{
    // Convert IP address string to a number.
    const auto addressParts = address.split(".");
    if (addressParts.size() != 4) {
        qCDebug(LOG_BASIC) << "isRfcLanRange: got a bad ipv4 address " << address;
        return true;
    }
    quint32 ip = 0;
    for (int i = 0; i < addressParts.size(); ++i)
        ip |= (addressParts[i].toUInt() & 0xff) << (i * 8);

    // Loopback address means a problem with local address detection.
    if (isRfcLoopbackAddress(ip)) {
        qCDebug(LOG_BASIC) << "isRfcLanRange: got a loopback ipv4 address " << address;
        return false;
    }
    return isRfcPrivateAddress(ip);
}
