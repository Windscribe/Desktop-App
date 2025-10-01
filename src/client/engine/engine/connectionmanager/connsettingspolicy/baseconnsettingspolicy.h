#pragma once

#include <QVector>
#include <QObject>
#include "api_responses/staticips.h"
#include "types/protocolstatus.h"
#include "engine/wireguardconfig/wireguardconfig.h"

enum CONNECTION_NODE_TYPE {
    CONNECTION_NODE_ERROR,
    CONNECTION_NODE_DEFAULT,
    CONNECTION_NODE_CUSTOM_CONFIG,
    CONNECTION_NODE_STATIC_IPS
};

struct CurrentConnectionDescr
{
    CONNECTION_NODE_TYPE connectionNodeType = CONNECTION_NODE_ERROR;

    // fields for CONNECTION_NODE_DEFAULT
    QString ip;
    uint port = 0;
    types::Protocol protocol;
    QString hostname;
    QString dnsHostName;
    QString verifyX509name;

    // fields for CONNECTION_NODE_CUSTOM_CONFIG
    QString ovpnData;
    QString customConfigFilename;
    QString remoteCmdLine;
    bool isAllowFirewallAfterConnection;

    // fields for WireGuard
    QString wgPeerPublicKey;
    QSharedPointer<WireGuardConfig> wgCustomConfig;

    // fields for static ips
    QString username;
    QString password;
    api_responses::StaticIpPortsVector staticIpPorts;
};

// helper class for ConnectionManager
class BaseConnSettingsPolicy : public QObject
{
    Q_OBJECT
public:
    BaseConnSettingsPolicy() : QObject(nullptr), bStarted_(false) {}
    virtual ~BaseConnSettingsPolicy() {}

    //virtual void startWith(QSharedPointer<const locationsmodel::BaseLocationInfo> mli, const ConnectionSettings &connectionSettings,
    //               const apiinfo::PortMap &portMap, bool isProxyEnabled);
    void start()
    {
        bStarted_ = true;
    }
    void stop()
    {
        bStarted_ = false;
    }

    virtual void reset() = 0;
    virtual void debugLocationInfoToLog() const = 0;
    virtual void putFailedConnection() = 0;
    virtual bool isFailed() const = 0;
    virtual CurrentConnectionDescr getCurrentConnectionSettings() const = 0;
    virtual bool isAutomaticMode() = 0;
    virtual bool isCustomConfig() = 0;
    virtual void resolveHostnames() = 0;
    virtual bool hasProtocolChanged() = 0;

signals:
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status);
    void hostnamesResolved();

protected:
    bool bStarted_;
};
