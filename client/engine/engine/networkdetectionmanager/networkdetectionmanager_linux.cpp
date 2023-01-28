#include "networkdetectionmanager_linux.h"

#include <QRegularExpression>
#include "utils/macutils.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <linux/wireless.h>

const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

NetworkDetectionManager_linux::NetworkDetectionManager_linux(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
{
    Q_UNUSED(helper);

    networkInterface_ = Utils::noNetworkInterface();
    getDefaultRouteInterface(isOnline_);
    updateNetworkInfo(false);

    ncm_ = QNetworkInformation::instance();
    connect(ncm_, &QNetworkInformation::isMeteredChanged, this, &NetworkDetectionManager_linux::isMeteredChanged);
    connect(ncm_, &QNetworkInformation::reachabilityChanged, this, &NetworkDetectionManager_linux::reachabilityChanged);
    connect(ncm_, &QNetworkInformation::transportMediumChanged, this, &NetworkDetectionManager_linux::transportMediumChanged);
}

NetworkDetectionManager_linux::~NetworkDetectionManager_linux()
{
}

void NetworkDetectionManager_linux::getCurrentNetworkInterface(ProtoTypes::NetworkInterface &networkInterface)
{
    networkInterface = networkInterface_;
}

bool NetworkDetectionManager_linux::isOnline()
{
    return isOnline_;
}

void NetworkDetectionManager_linux::isMeteredChanged(bool _isMetered)
{
    updateNetworkInfo(true);
}

void NetworkDetectionManager_linux::reachabilityChanged(QNetworkInformation::Reachability _newReachability)
{
    updateNetworkInfo(true);
}

void NetworkDetectionManager_linux::transportMediumChanged(QNetworkInformation::TransportMedium _current)
{
    updateNetworkInfo(true);
}

void NetworkDetectionManager_linux::updateNetworkInfo(bool bWithEmitSignal)
{
    bool newIsOnline;
    QString ifname = getDefaultRouteInterface(newIsOnline);

    if (isOnline_ != newIsOnline)
    {
        isOnline_ = newIsOnline;
        emit onlineStateChanged(isOnline_);
    }


    ProtoTypes::NetworkInterface newNetworkInterface = Utils::noNetworkInterface();
    if (!ifname.isEmpty())
    {
        getInterfacePars(ifname, newNetworkInterface);
    }

    if (!google::protobuf::util::MessageDifferencer::Equals(newNetworkInterface, networkInterface_))
    {
        networkInterface_ = newNetworkInterface;
        if (bWithEmitSignal)
        {
            emit networkChanged(networkInterface_);
        }
    }
}

QString NetworkDetectionManager_linux::getDefaultRouteInterface(bool &isOnline)
{
    QString strReply;
    FILE *file = popen("/etc/windscribe/route -n | grep '^0\\.0\\.0\\.0'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);

    isOnline = !lines.isEmpty();

    for (auto &it : lines)
    {
        const QStringList pars = it.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (pars.size() == 8)
        {
            if (!pars[7].startsWith("tun") && !pars[7].startsWith("utun"))
            {
                return pars[7];
            }
        }
        else
        {
            qDebug(LOG_BASIC) << "NetworkDetectionManager_linux::getDefaultRouteInterface parse error";
            return QString();
        }
    }
    return QString();
}

void NetworkDetectionManager_linux::getInterfacePars(const QString &ifname, ProtoTypes::NetworkInterface &outNetworkInterface)
{
    outNetworkInterface.set_interface_name(ifname.toStdString().c_str());
    outNetworkInterface.set_interface_index(if_nametoindex(ifname.toStdString().c_str()));
    QString macAddress = getMacAddressByIfName(ifname);
    outNetworkInterface.set_physical_address(macAddress.toStdString().c_str());

    bool isWifi = checkWirelessByIfName(ifname);
    if (isWifi)
    {
        outNetworkInterface.set_interface_type(ProtoTypes::NETWORK_INTERFACE_WIFI);
        QString friendlyName = getFriendlyNameByIfName(ifname);
        if (!friendlyName.isEmpty())
        {
            outNetworkInterface.set_network_or_ssid(friendlyName.toStdString().c_str());
        }
        else
        {
            outNetworkInterface.set_network_or_ssid(macAddress.toStdString().c_str());
        }
    }
    else
    {
        outNetworkInterface.set_interface_type(ProtoTypes::NETWORK_INTERFACE_ETH);
        QString friendlyName = getFriendlyNameByIfName(ifname);
        if (!friendlyName.isEmpty())
        {
            outNetworkInterface.set_network_or_ssid(friendlyName.toStdString().c_str());
        }
        else
        {
            outNetworkInterface.set_network_or_ssid(macAddress.toStdString().c_str());
        }
    }

    outNetworkInterface.set_active(isActiveByIfName(ifname));
}

QString NetworkDetectionManager_linux::getMacAddressByIfName(const QString &ifname)
{
    QString ret;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd != -1)
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , ifname.toStdString().c_str() , IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
        {
           unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
           // format mac address
           ret = QString::asprintf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
       }
       close(fd);
    }
    return ret;
}

bool NetworkDetectionManager_linux::isActiveByIfName(const QString &ifname)
{
    bool ret = false;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd != -1)
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , ifname.toStdString().c_str() , IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
        {
           ret = (ifr.ifr_flags & ( IFF_UP | IFF_RUNNING )) == ( IFF_UP | IFF_RUNNING );
        }
        close(fd);
    }
    return ret;
}

bool NetworkDetectionManager_linux::checkWirelessByIfName(const QString &ifname)
{
    bool ret = false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd != -1)
    {
        struct iwreq pwrq;
        memset(&pwrq, 0, sizeof(pwrq));
        strncpy(pwrq.ifr_name, ifname.toStdString().c_str(), IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIWNAME, &pwrq) != -1)
        {
            ret = true;
        }
        close(fd);
    }
    return ret;
}

QString NetworkDetectionManager_linux::getFriendlyNameByIfName(const QString &ifname)
{
    QString strReply;
    FILE *file = popen("nmcli -t -f NAME,DEVICE c show", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    for (auto &it : lines)
    {
        const QStringList pars = it.split(':', Qt::SkipEmptyParts);
        if (pars.size() == 2)
        {
            if (pars[1] == ifname)
            {
                return pars[0];
            }
        }
    }
    return QString();
}

