#include "makeovpnfile.h"

#include <QString>

#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

#if defined (Q_OS_WIN)
#include "types/global_consts.h"
#endif

MakeOVPNFile::MakeOVPNFile()
{
}

MakeOVPNFile::~MakeOVPNFile()
{
}

bool MakeOVPNFile::generate(const QString &ovpnData, const QString &ip, types::Protocol protocol, uint port,
                            uint portForStunnelOrWStunnel, int mss, const QString &defaultGateway,
                            const QString &openVpnX509, const QString &customDns, bool isAntiCensorship,
                            bool isEmergencyConnect)
{
#ifdef Q_OS_WIN
    Q_UNUSED(defaultGateway);
#endif

    QString strExtraConfig = ExtraConfig::instance().getExtraConfigForOpenVpn();
    bool bExtraContainsRemote = !ExtraConfig::instance().getRemoteIpFromExtraConfig().isEmpty();

    config_ = ExtraConfig::instance().modifyVerbParameter(ovpnData, strExtraConfig);

    // Set timeout to 30s according to this: https://www.notion.so/windscribe/Data-Plane-VPN-Protocol-Failover-Refresh-48ed7aea1a244617b327c3a7d816a902
    config_ += "\r\nconnect-timeout 30\r\n";

#if defined (Q_OS_WIN)
    // NOTE: --dev tun option already included in ovpnData by the server API.
    // NOTE: the emergency connect OpenVPN server is old-old and generates data packets not supported by the DCO driver.
    // We use the --dev-node option to ensure OpenVPN will only use the dco/wintun adapter instance we create and not
    // possibly attempt to use an adapter created by other software (e.g. the vanilla OpenVPN client app).
    config_ += QString("\r\ndev-node %1\r\n").arg(kOpenVPNAdapterIdentifier);
    if (!isEmergencyConnect && ExtraConfig::instance().useOpenVpnDCO()) {
        config_ += "\r\nwindows-driver ovpn-dco\r\n";
        // DCO driver on Windows will not accept the AES-256-CBC cipher and will drop back to using wintun if it is provided in the ciphers list.
        config_.replace(":AES-256-CBC:", ":");
    } else {
        config_ += "\r\nwindows-driver wintun\r\n";
    }
#endif

    if (protocol == types::Protocol::OPENVPN_UDP) {
        if (!bExtraContainsRemote) {
            config_ += "\r\nremote " + ip;
        }
        config_ += "\r\nport " + QString::number(port);
        config_ += "\r\nproto udp\r\n";

        if (mss > 0) {
            config_ += QString("mssfix %1\r\n").arg(mss);
        }
    } else if (protocol == types::Protocol::OPENVPN_TCP) {
        if (!bExtraContainsRemote) {
            config_ += "\r\nremote " + ip;
        }
        config_ += "\r\nport " + QString::number(port);
        config_ += "\r\nproto tcp\r\n";
    } else if (protocol.isStunnelOrWStunnelProtocol()) {
        if (!bExtraContainsRemote) {
            config_ += "\r\nremote 127.0.0.1";
        }
        config_ += QString("\r\nport %1").arg(portForStunnelOrWStunnel);
        config_ += "\r\nproto tcp\r\n";
#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
        if (!defaultGateway.isEmpty()) {
            qCInfo(LOG_CONNECTION) << "defaultGateway for stunnel/wstunnel ovpn config: " << defaultGateway;
            config_ += QString("route %1 255.255.255.255 %2\r\n").arg(ip).arg(defaultGateway);
        }
#endif
    } else {
        WS_ASSERT(false);
    }

    if (openVpnX509 != "") {
        config_ += QString("verify-x509-name %1 name\r\n").arg(openVpnX509);
    }

    if (!customDns.isEmpty()) {
        config_ += "\r\npull-filter ignore \"dhcp-option DNS\"\r\n";
        config_ += QString("dhcp-option DNS %1\r\n").arg(customDns);
    }

    // concatenate with windscribe_extra.conf file, if it exists
    if (!strExtraConfig.isEmpty()) {
        qCInfo(LOG_CONNECTION) << "Adding extra options to OVPN config:" << strExtraConfig;
        config_ += strExtraConfig;
    }

    if (isAntiCensorship) {
        config_ += "udp-stuffing\n";
        config_ += "tcp-split-reset\n";
    }

    return true;
}
