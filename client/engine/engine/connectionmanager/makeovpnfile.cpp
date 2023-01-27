#include "makeovpnfile.h"
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include "utils/extraconfig.h"
#include "utils/logger.h"
#include "../openvpnversioncontroller.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/tempscripts_mac.h"
#elif defined(Q_OS_LINUX)
    #include "utils/dnsscripts_linux.h"
#endif

MakeOVPNFile::MakeOVPNFile()
{
    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(strPath);
    dir.mkpath(strPath);
    path_ = strPath + "/config.ovpn";
    file_.setFileName(path_);
}

MakeOVPNFile::~MakeOVPNFile()
{
    file_.close();
    file_.remove();
}

bool MakeOVPNFile::generate(const QString &ovpnData, const QString &ip, const ProtocolType &protocol, uint port,
                            uint portForStunnelOrWStunnel, int mss, const QString &defaultGateway, const QString &openVpnX509, bool blockOutsideDnsOption)
{
#ifndef Q_OS_MAC
    Q_UNUSED(defaultGateway);
#endif

    if (!file_.isOpen())
    {
        if (!file_.open(QIODeviceBase::WriteOnly))
        {
            qCDebug(LOG_CONNECTION) << "Can't open config file:" << file_.fileName();
            return false;
        }
    }

    file_.resize(0);

    QString strExtraConfig = ExtraConfig::instance().getExtraConfigForOpenVpn();
    bool bExtraContainsRemote = strExtraConfig.contains("remote", Qt::CaseInsensitive);


    QString newOvpnData = ExtraConfig::instance().modifyVerbParameter(ovpnData, strExtraConfig);


    file_.write( newOvpnData.toLocal8Bit());

    QString str;

#ifdef Q_OS_WIN

    if(blockOutsideDnsOption) {
        str = "\r\nblock-outside-dns\r\n";
        file_.write(str.toLocal8Bit());
    }

    if (OpenVpnVersionController::instance().isUseWinTun())
    {
        str = "\r\nwindows-driver wintun\r\n";
        file_.write(str.toLocal8Bit());
    }

#endif

    if (protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_UDP)
    {
        if (!bExtraContainsRemote)
        {
            str = "\r\nremote " + ip;
            file_.write(str.toLocal8Bit());
        }
        str = "\r\nport " + QString::number(port);
        file_.write(str.toLocal8Bit());

        str = "\r\nproto udp\r\n";
        file_.write(str.toLocal8Bit());

        if (mss > 0)
        {
            str = QString("mssfix %1\r\n").arg(mss);
            file_.write(str.toLocal8Bit());
        }
    }
    else if (protocol.getType() == ProtocolType::PROTOCOL_OPENVPN_TCP)
    {
        if (!bExtraContainsRemote)
        {
            str = "\r\nremote " + ip;
            file_.write(str.toLocal8Bit());
        }
        str = "\r\nport " + QString::number(port);
        file_.write(str.toLocal8Bit());

        str = "\r\nproto tcp\r\n";
        file_.write(str.toLocal8Bit());
    }
    else if (protocol.isStunnelOrWStunnelProtocol())
    {
        if (!bExtraContainsRemote)
        {
            str = "\r\nremote 127.0.0.1";
            file_.write(str.toLocal8Bit());
        }
        str = QString("\r\nport %1").arg(portForStunnelOrWStunnel);
        file_.write(str.toLocal8Bit());

        str = "\r\nproto tcp\r\n";
        file_.write(str.toLocal8Bit());

    #if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
        if (!defaultGateway.isEmpty())
        {
            qCDebug(LOG_CONNECTION) << "defaultGateway for stunnel/wstunnel ovpn config: " << defaultGateway;
            str = QString("route %1 255.255.255.255 %2\r\n").arg(ip).arg(defaultGateway);
            file_.write(str.toLocal8Bit());
        }
    #endif

    }
    else
    {
        Q_ASSERT(false);
    }

    if (openVpnX509 != "")
    {
        str = QString("verify-x509-name %1 name\r\n").arg(openVpnX509);
        file_.write(str.toLocal8Bit());
    }

#ifdef Q_OS_MAC
    str = "--script-security 2\r\n";
    file_.write(str.toLocal8Bit());

    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    if (strDnsPath.isEmpty()) {
        return false;
    }
    QString cmd1 = "\nup \"" + strDnsPath + " -up\"\n";
    file_.write(cmd1.toUtf8());
#elif defined(Q_OS_LINUX)
    str = "--script-security 2\r\n";
    file_.write(str.toLocal8Bit());

    QString dnsScript = DnsScripts_linux::instance().scriptPath();
    QString cmd1 = "\nup " + dnsScript  + "\n";
    QString cmd2 = "down " + dnsScript + "\n";
    QString cmd3 = "down-pre\n";
    QString cmd4 = "dhcp-option DOMAIN-ROUTE .\n";   // prevent DNS leakage  and without it doesn't work update-systemd-resolved script
    file_.write(cmd1.toUtf8());
    file_.write(cmd2.toUtf8());
    file_.write(cmd3.toUtf8());
    file_.write(cmd4.toUtf8());
#endif

    // concatenate with windscribe_extra.conf file, if it exists
    if (!strExtraConfig.isEmpty())
    {
        qCDebug(LOG_CONNECTION) << "Adding extra options to OVPN config:" << strExtraConfig;
        file_.write(strExtraConfig.toUtf8());
    }

    file_.flush();

    //qCDebug(LOG_CONNECTION) << "OVPN-config path:" << path_;
    return true;
}
