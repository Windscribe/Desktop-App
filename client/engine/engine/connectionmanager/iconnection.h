#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <QThread>
#include "types/proxysettings.h"
#include "types/enums.h"
#include "adaptergatewayinfo.h"

class IHelper;
class WireGuardConfig;

enum class ConnectionType { IKEV2, OPENVPN, WIREGUARD };

class IConnection : public QThread
{
    Q_OBJECT

public:
    explicit IConnection(QObject *parent): QThread(parent) {}
    virtual ~IConnection() {}

    // config contents for openvpn, url for ikev2
    virtual void startConnect(const QString &configOrUrl, const QString &ip, const QString &dnsHostName,
                              const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                              const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression,
                              bool isAutomaticConnectionMode, bool isCustomConfig, const QString &overrideDnsIp) = 0;
    virtual void startDisconnect() = 0;
    virtual bool isDisconnected() const = 0;
    virtual ConnectionType getConnectionType() const = 0;
    virtual bool isAllowFirewallAfterCustomConfigConnection() const { return true; }

    virtual void continueWithUsernameAndPassword(const QString &username, const QString &password) = 0;
    virtual void continueWithPassword(const QString &password) = 0;

signals:
    void connected(const AdapterGatewayInfo &connectionAdapterInfo);
    void disconnected();
    void reconnecting();
    void error(CONNECT_ERROR err);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void interfaceUpdated(const QString &interfaceName);  // WireGuard-specific.

    void requestUsername();
    void requestPassword();
};

#endif // ICONNECTION_H
