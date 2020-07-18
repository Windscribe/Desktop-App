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
#endif

MakeOVPNFile::MakeOVPNFile()
{
    QString strPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
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

bool MakeOVPNFile::generate(const QByteArray &ovpnData, const QString &ip, const ProtocolType &protocol, uint port, uint portForStunnel, uint portForWstunnel, int mss)
{
    if (!file_.isOpen())
    {
        if (!file_.open(QIODevice::WriteOnly))
        {
            qCDebug(LOG_CONNECTION) << "Can't open config file:" << file_.fileName();
            return false;
        }
    }

    file_.resize(0);

    QString strExtraConfig = ExtraConfig::instance().getExtraConfigForOpenVpn();
    bool bExtraContainsRemote = strExtraConfig.contains("remote", Qt::CaseInsensitive);


    QByteArray newOvpnData = ExtraConfig::instance().modifyVerbParameter(ovpnData, strExtraConfig);


    file_.write( newOvpnData );

    QString str;

#ifdef Q_OS_WIN
    str = "\r\nblock-outside-dns\r\n";
    file_.write(str.toLocal8Bit());

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

        if (mss != -1)
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
    else if (protocol.getType() == ProtocolType::PROTOCOL_STUNNEL)
    {
        if (!bExtraContainsRemote)
        {
            str = "\r\nremote 127.0.0.1";
            file_.write(str.toLocal8Bit());
        }
        str = QString("\r\nport %1").arg(portForStunnel);
        file_.write(str.toLocal8Bit());

        str = "\r\nproto tcp\r\n";
        file_.write(str.toLocal8Bit());

    #ifdef Q_OS_MAC
        QString defaultGateway = MacUtils::getDefaultGatewayForPrimaryInterface();
        if (!defaultGateway.isEmpty())
        {
            qCDebug(LOG_CONNECTION) << "defaultGateway for stunnel ovpn config: " << defaultGateway;
            str = QString("route %1 255.255.255.255 %2\r\n").arg(ip).arg(defaultGateway);
            file_.write(str.toLocal8Bit());
        }
    #endif

    }
    else if (protocol.getType() == ProtocolType::PROTOCOL_WSTUNNEL)
    {
        if (!bExtraContainsRemote)
        {
            str = "\r\nremote 127.0.0.1";
            file_.write(str.toLocal8Bit());
        }
        str = QString("\r\nport %1").arg(portForWstunnel);
        file_.write(str.toLocal8Bit());

        str = "\r\nproto tcp\r\n";
        file_.write(str.toLocal8Bit());

    #ifdef Q_OS_MAC
        QString defaultGateway = MacUtils::getDefaultGatewayForPrimaryInterface();
        if (!defaultGateway.isEmpty())
        {
            qCDebug(LOG_CONNECTION) << "defaultGateway for wstunnel ovpn config: " << defaultGateway;
            str = QString("route %1 255.255.255.255 %2\r\n").arg(ip).arg(defaultGateway);
            file_.write(str.toLocal8Bit());
        }
    #endif

    }
    else
    {
        Q_ASSERT(false);
    }

#ifdef Q_OS_MAC
    str = "--script-security 2\r\n";
    file_.write(str.toLocal8Bit());

    // for Mac not add default gateway
    // routes are managed in helper depending split tunneling options
    //str = "route-noexec\r\n";
    //file_.write(str.toLocal8Bit());

    QString strDnsPath = TempScripts_mac::instance().dnsScriptPath();
    QString cmd1 = "\nup \"" + strDnsPath + " -up\"\n";
    file_.write(cmd1.toUtf8());
#endif

    // concatenate with windscribe_extra.conf file, if it exists
    if (!strExtraConfig.isEmpty())
    {
        file_.write(strExtraConfig.toUtf8());
    }

    file_.flush();

    //qCDebug(LOG_CONNECTION) << "OVPN-config path:" << path_;
    return true;
}
