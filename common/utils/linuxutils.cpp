#include "linuxutils.h"
#include "utils.h"
#include <sys/utsname.h>
#include <net/if.h>
#include <unistd.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <memory>

#include <QCoreApplication>
#include <QFile>
#include <QHostAddress>
#include <QRegularExpression>

#include "logger.h"
#include "wsscopeguard.h"

namespace LinuxUtils
{

const int MAX_NETLINK_SIZE = 8192;

QString getOsVersionString()
{
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        return QString(unameData.sysname) + " " + QString(unameData.version);
    }
    else
    {
        return "Can't detect OS Linux version";
    }
}

void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp)
{
    outInterfaceName.clear();
    outGatewayIp.clear();
    outAdapterIp.clear();

    int lowestMetric = INT32_MAX;

    QList<RoutingTableEntry> entries = getRoutingTable(false);
    for (const RoutingTableEntry& entry : qAsConst(entries))
    {
        if (entry.isIPv4() && !entry.source.isEmpty() && entry.metric < lowestMetric)
        {
            lowestMetric = entry.metric;
            outInterfaceName = entry.interface;
            outAdapterIp = entry.source;
        }
    }

    if (lowestMetric != INT32_MAX)
    {
        for (const RoutingTableEntry& entry : qAsConst(entries))
        {
            if (entry.metric == lowestMetric && entry.isIPv4() && !entry.gateway.isEmpty())
            {
                outGatewayIp = entry.gateway;
                break;
            }
        }
    }
}

QString getLinuxKernelVersion()
{
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        QRegularExpression re("(\\d+\\.\\d+(\\.\\d+)*)");
        QRegularExpressionMatch match = re.match(unameData.release);
        if(match.hasMatch()) {
            return match.captured(1);
        }
    }

    return QString("Can't detect Linux Kernel version");
}

const QString getLastInstallPlatform()
{
    static QString linuxPlatformName;
    static bool tried = false;

    // only read in file once, cache the result
    if (tried) return linuxPlatformName;
    tried = true;

    if (!QFile::exists(LAST_INSTALL_PLATFORM_FILE))
    {
        qCDebug(LOG_BASIC) << "Couldn't find previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    QFile lastInstallPlatform(LAST_INSTALL_PLATFORM_FILE);

    if (!lastInstallPlatform.open(QIODeviceBase::ReadOnly))
    {
        qCDebug(LOG_BASIC) << "Couldn't open previous install platform file: " << LAST_INSTALL_PLATFORM_FILE;
        return "";
    }

    linuxPlatformName = lastInstallPlatform.readAll().trimmed(); // remove newline
    return linuxPlatformName;
}

bool isGuiAlreadyRunning()
{
    // Look for process containing "Windscribe" -- exclude grep and Engine
    QString cmd = "ps axco command | grep Windscribe | grep -v grep | grep -v WindscribeEngine | grep -v windscribe-cli";
    QString response = Utils::execCmd(cmd);
    return response.trimmed() != "";
}

QString getLocalIP()
{
    // Yegor and Clayton found this command to work on many distros, including old ones.
    QString sLocalIP = Utils::execCmd("hostname -I | awk '{print $1}'").trimmed();

    // Check if we received a valid IPv4 address.
    QHostAddress addr;
    if (addr.setAddress(sLocalIP) && addr.protocol() == QAbstractSocket::IPv4Protocol) {
        return sLocalIP;
    }

    qCDebug(LOG_BASIC) << "LinuxUtils::getLocalIP() hostname failed:" << sLocalIP;

    // Try to retrieve the address via the Netlink API.
    sLocalIP.clear();
    int lowestMetric = INT32_MAX;

    QList<RoutingTableEntry> entries = getRoutingTable(false);
    for (const RoutingTableEntry& entry : qAsConst(entries))
    {
        if (entry.isIPv4() && !entry.source.isEmpty() && entry.metric < lowestMetric)
        {
            lowestMetric = entry.metric;
            sLocalIP = entry.source;
        }
    }

    if (sLocalIP.isEmpty()) {
        qCDebug(LOG_BASIC) << "LinuxUtils::getLocalIP() failed to determine the local IP via Netlink";
    }

    return sLocalIP;
}

static QString getNetlinkIP(int family, const char* buffer)
{
    char dst[INET6_ADDRSTRLEN] = {0};

    if (inet_ntop(family, buffer, dst, INET6_ADDRSTRLEN) == nullptr)
    {
        qCDebug(LOG_BASIC) << "LinuxUtils::getNetlinkIP() unsupported address family:" << family;
        return QString("");
    }

    return QString::fromLatin1(dst).trimmed();
}

static void readNetlink(int socket_fd, int seq, char* output, size_t& size)
{
    const int MAX_NETLINK_ATTEMPTS = 8;
    struct nlmsghdr* nl_hdr = nullptr;

    size_t message_size = 0;
    do
    {
        size_t latency = 0;
        size_t total_bytes = 0;
        ssize_t bytes = 0;
        while (bytes == 0)
        {
            bytes = recv(socket_fd, output, MAX_NETLINK_SIZE - message_size, 0);
            if (bytes < 0) {
                throw std::system_error(errno, std::generic_category(), "could not read from Netlink socket");
            }

            total_bytes += bytes;
            if (latency >= MAX_NETLINK_ATTEMPTS) {
                throw std::system_error(errno, std::generic_category(), "Netlink socket timeout");
            }

            if (bytes == 0)
            {
                if (total_bytes > 0) {
                    // Bytes were read, but now no more are available, attempt to parse
                    // the received NETLINK message.
                    break;
                }
                ::usleep(20);
                latency += 1;
            }
        }

        // Assure valid header response, and not an error type.
        nl_hdr = (struct nlmsghdr*)output;
        if (NLMSG_OK(nl_hdr, bytes) == 0 || nl_hdr->nlmsg_type == NLMSG_ERROR) {
            throw std::system_error(errno, std::generic_category(), "read invalid Netlink message");
        }

        if (nl_hdr->nlmsg_type == NLMSG_DONE) {
            break;
        }

        output += bytes;
        message_size += bytes;
        if ((nl_hdr->nlmsg_flags & NLM_F_MULTI) == 0) {
            break;
        }
    } while (static_cast<pid_t>(nl_hdr->nlmsg_seq) != seq ||
             static_cast<pid_t>(nl_hdr->nlmsg_pid) != getpid());

    size = message_size;
}

static void getNetlinkRoutes(const struct nlmsghdr* netlink_msg, QList<RoutingTableEntry>& entries,
                             bool includeZeroMetricEntries)
{
    int mask = 0;
    char interface[IF_NAMESIZE] = {0};

    struct rtmsg* message = static_cast<struct rtmsg*>(NLMSG_DATA(netlink_msg));
    struct rtattr* attr = static_cast<struct rtattr*>(RTM_RTA(message));
    uint32_t attr_size = RTM_PAYLOAD(netlink_msg);

    RoutingTableEntry entry;

    // Iterate over each route in the netlink message
    bool has_destination = false;
    while (RTA_OK(attr, attr_size))
    {
        switch (attr->rta_type)
        {
        case RTA_OIF:
            if_indextoname(*(int*)RTA_DATA(attr), interface);
            entry.interface = QString::fromLatin1(interface);
            break;
        case RTA_GATEWAY:
            entry.gateway = getNetlinkIP(message->rtm_family, (char*)RTA_DATA(attr));
            break;
        case RTA_PREFSRC:
            entry.source = getNetlinkIP(message->rtm_family, (char*)RTA_DATA(attr));
            break;
        case RTA_DST:
            if (message->rtm_dst_len != 32 && message->rtm_dst_len != 128) {
                mask = (int)message->rtm_dst_len;
            }
            entry.destination = getNetlinkIP(message->rtm_family, (char*)RTA_DATA(attr));
            has_destination = true;
            break;
        case RTA_PRIORITY:
            entry.metric = (*(int*)RTA_DATA(attr));
            break;
        case RTA_METRICS:
            struct rtattr* xattr = static_cast<struct rtattr*> RTA_DATA(attr);
            auto xattr_size = RTA_PAYLOAD(attr);
            while (RTA_OK(xattr, xattr_size)) {
                switch (xattr->rta_type) {
                case RTAX_MTU:
                    entry.mtu = *reinterpret_cast<int*>(RTA_DATA(xattr));
                    break;
                case RTAX_HOPLIMIT:
                    entry.hopcount = *reinterpret_cast<int*>(RTA_DATA(xattr));
                    break;
                }
                xattr = RTA_NEXT(xattr, xattr_size);
            }
            break;
        }
        attr = RTA_NEXT(attr, attr_size);
    }

    if (!has_destination)
    {
        switch (message->rtm_family)
        {
        case AF_INET:
            entry.destination = "0.0.0.0";
            break;
        case AF_INET6:
            entry.destination = "::";
            break;
        default:
            break;
        }

        if (message->rtm_dst_len) {
            mask = (int)message->rtm_dst_len;
        }
    }

    // Route type determination
    if (message->rtm_type == RTN_UNICAST) {
        entry.type = "gateway";
    } else if (message->rtm_type == RTN_LOCAL) {
        entry.type = "local";
    } else if (message->rtm_type == RTN_BROADCAST) {
        entry.type = "broadcast";
    } else if (message->rtm_type == RTN_ANYCAST) {
        entry.type = "anycast";
    } else {
        entry.type = "other";
    }

    entry.flags   = message->rtm_flags;
    entry.netmask = mask;
    entry.family  = message->rtm_family;

    if (entry.metric > 0 || includeZeroMetricEntries) {
        entries.append(entry);
    }
}

// Adapted from routes.cpp in the osquery project
QList<RoutingTableEntry> getRoutingTable(bool includeZeroMetricEntries)
{
    QList<RoutingTableEntry> entries;

    try
    {
        int socket_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
        if (socket_fd < 0) {
            throw std::system_error(errno, std::generic_category(), "failed to create Netlink socket endpoint");
        }

        auto exitGuard = wsl::wsScopeGuard([&]
        {
            close(socket_fd);
        });

        std::unique_ptr<unsigned char[]> netlink_buffer(new unsigned char[MAX_NETLINK_SIZE]);

        memset(netlink_buffer.get(), 0, MAX_NETLINK_SIZE);
        auto netlink_msg = (struct nlmsghdr*)netlink_buffer.get();
        netlink_msg->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
        netlink_msg->nlmsg_type  = RTM_GETROUTE; // routes from kernel routing table
        netlink_msg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST | NLM_F_ATOMIC;
        netlink_msg->nlmsg_seq   = 0;
        netlink_msg->nlmsg_pid   = getpid();

        if (send(socket_fd, netlink_msg, netlink_msg->nlmsg_len, 0) < 0) {
            throw std::system_error(errno, std::generic_category(), "send failed");
        }

        size_t size = 0;
        readNetlink(socket_fd, 1, (char*)netlink_msg, size);

        while (NLMSG_OK(netlink_msg, size))
        {
            getNetlinkRoutes(netlink_msg, entries, includeZeroMetricEntries);
            netlink_msg = NLMSG_NEXT(netlink_msg, size);
        }
    }
    catch (std::system_error& ex)
    {
        qCDebug(LOG_BASIC) << "LinuxUtils::getRoutingTable()" << ex.what();
    }

    return entries;
}

bool RoutingTableEntry::isIPv4() const
{
    return (family == AF_INET);
}

} // end namespace LinuxUtils
