#pragma once

#include <variant>
#include <QThread>
#include "types/proxysettings.h"
#include "types/enums.h"
#include "adaptergatewayinfo.h"

class WireGuardConfig;

enum class ConnectionType { IKEV2, OPENVPN, WIREGUARD };

struct OpenVpnStartParams
{
    QString config;
    QString username;
    QString password;
    types::ProxySettings proxySettings;
    bool isCustomConfig = false;
};

struct WireGuardStartParams
{
    const WireGuardConfig *wireGuardConfig = nullptr;
    QString overrideDnsIp;
};

struct Ikev2StartParams
{
    QString hostname;
    QString ip;
    QString dnsHostName;
    QString username;
    QString password;
    bool isEnableCompression = false;
    QString overrideDnsIp;
};

using StartConnectParams = std::variant<OpenVpnStartParams, WireGuardStartParams, Ikev2StartParams>;

class IConnection : public QThread
{
    Q_OBJECT

public:
    explicit IConnection(QObject *parent): QThread(parent) {}
    virtual ~IConnection() {}

    virtual void startConnect(const StartConnectParams &params) = 0;
    virtual void startDisconnect() = 0;
    virtual bool isDisconnected() const = 0;
    virtual void waitForDisconnect() {}
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
