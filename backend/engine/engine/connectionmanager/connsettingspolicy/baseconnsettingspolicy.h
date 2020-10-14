#ifndef BASECONNSETTINGSPOLICY_H
#define BASECONNSETTINGSPOLICY_H

#include "engine/apiinfo/staticips.h"
#include "engine/apiinfo/portmap.h"
#include "engine/types/wireguardconfig.h"

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
    ProtocolType protocol;
    QString hostname;
    QString dnsHostName;

    // fields for CONNECTION_NODE_CUSTOM_CONFIG
    QString ovpnData;
    QString customConfigFilename;
    QString remoteCmdLine;

    // fields for WireGuard
    QString wgPublicKey;
    QSharedPointer<WireGuardConfig> wgCustomConfig;

    // fields for static ips
    QString username;
    QString password;
    apiinfo::StaticIpPortsVector staticIpPorts;
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
    virtual void saveCurrentSuccessfullConnectionSettings() = 0;
    virtual bool isAutomaticMode() = 0;
    virtual void resolveHostnames() = 0;

signals:
    void hostnamesResolved();

protected:
    bool bStarted_;
};

#endif // BASECONNSETTINGSPOLICY_H
