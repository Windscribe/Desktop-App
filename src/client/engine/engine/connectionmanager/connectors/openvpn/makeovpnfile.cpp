#include "makeovpnfile.h"

#include <QString>

#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/ws_assert.h"
#include "api_responses/amneziawgunblockparams.h"
#include <wsnet/WSNet.h>

MakeOVPNFile::MakeOVPNFile()
{
}

MakeOVPNFile::~MakeOVPNFile()
{
}

bool MakeOVPNFile::generate(const QString &ovpnData, const QString &ip, types::Protocol protocol, uint port,
                            uint portForStunnelOrWStunnel, int mss, const QString &defaultGateway,
                            const QString &openVpnX509, const QString &customDns, bool isAntiCensorship,
                            const QString &awgPreset)
{
#ifdef Q_OS_WIN
    Q_UNUSED(defaultGateway);
#endif

    if (!NetworkingValidation::isIp(ip)) {
        qCCritical(LOG_CONNECTION) << "MakeOVPNFile::generate: refusing to build config, node IP is not a valid IP address";
        return false;
    }

    QString strExtraConfig = ExtraConfig::instance().getExtraConfigForOpenVpn();
    bool bExtraContainsRemote = !ExtraConfig::instance().getRemoteIpFromExtraConfig().isEmpty();

    config_ = ovpnData;

    // Set timeout to 30s according to this: https://www.notion.so/windscribe/Data-Plane-VPN-Protocol-Failover-Refresh-48ed7aea1a244617b327c3a7d816a902
    config_ += "\r\nconnect-timeout 30\r\n";

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
        config_ += "\r\nproto tcp-client\r\n";
    } else if (protocol.isStunnelOrWStunnelProtocol()) {
        if (!bExtraContainsRemote) {
            config_ += "\r\nremote 127.0.0.1";
        }
        config_ += QString("\r\nport %1").arg(portForStunnelOrWStunnel);
        config_ += "\r\nproto tcp-client\r\n";
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

    // concatenate with advanced parameters file, if it exists
    if (!strExtraConfig.isEmpty()) {
        qCInfo(LOG_CONNECTION) << "Adding extra options to OVPN config:" << strExtraConfig;
        config_ += strExtraConfig;
    }

    if (isAntiCensorship) {
        config_ += "udp-stuffing\n";
        config_ += "tcp-split-reset\n";

        // Add I1-I5 parameters if available
        // NOTE: Emergency connect supplies empty string here
        if (!awgPreset.isEmpty()) {
            api_responses::AmneziawgUnblockParams unblockParams(
                wsnet::WSNet::instance()->apiResourcersManager()->amneziawgUnblockParams());

            if (unblockParams.isValid()) {
                api_responses::AmneziawgUnblockParam param =
                    unblockParams.getUnblockParamForPreset(awgPreset);

                // Add I1-I5 parameters (in order, only if they exist)
                for (int i = 0; i < param.iValues.size() && i < 5; ++i) {
                    if (!param.iValues[i].isEmpty()) {
                        // Sanitize iValue: allow only alphanumeric, space, digits, and angle brackets
                        // Strip newlines and other special characters
                        QString sanitizedValue;
                        for (const QChar &ch : param.iValues[i]) {
                            if (ch.isLetterOrNumber() || ch.isSpace() || ch == '<' || ch == '>') {
                                sanitizedValue += ch;
                            }
                        }
                        config_ += QString("i%1 \"%2\"\n").arg(i + 1).arg(sanitizedValue);
                    }
                }

                if (!param.iValues.isEmpty()) {
                    qCInfo(LOG_CONNECTION) << "Added" << param.iValues.size()
                                        << "I-parameters to OpenVPN config from preset:"
                                        << awgPreset;
                }
            }
        }
    }

    return true;
}
