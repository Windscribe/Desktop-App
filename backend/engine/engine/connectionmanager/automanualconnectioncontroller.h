#ifndef AUTOMANUALCONNECTIONCONTROLLER_H
#define AUTOMANUALCONNECTIONCONTROLLER_H

#include "engine/types/connectionsettings.h"
#include "engine/locationsmodel/mutablelocationinfo.h"
#include "engine/apiinfo/staticips.h"
#include "engine/apiinfo/portmap.h"

// helper class for ConnectionManager
// manage automatic and manual connection mode
class AutoManualConnectionController
{
public:

    enum CONNECTION_NODE_TYPE { CONNECTION_NODE_ERROR, CONNECTION_NODE_DEFAULT, CONNECTION_NODE_CUSTOM_OVPN_CONFIG,
                              CONNECTION_NODE_STATIC_IPS};

    struct CurrentConnectionDescr
    {
        CONNECTION_NODE_TYPE connectionNodeType = CONNECTION_NODE_ERROR;

        // fields for CONNECTION_NODE_DEFAULT
        QString ip;
        uint port = 0;
        ProtocolType protocol;
        QString hostname;
        QString dnsHostName;

        // fields for CONNECTION_NODE_CUSTOM_OVPN_CONFIG
        QString pathOvpnConfigFile;

        // fields for WireGuard
        QString wgPublicKey;

        // fields for static ips
        QString username;
        QString password;
        apiinfo::StaticIpPortsVector staticIpPorts;
    };

    AutoManualConnectionController();

    void startWith(QSharedPointer<locationsmodel::MutableLocationInfo> mli, const ConnectionSettings &connectionSettings,
                   const apiinfo::PortMap &portMap, bool isProxyEnabled);
    void reset();
    void stop();
    void debugLocationInfoToLog();
    void putFailedConnection();
    bool isFailed();
    CurrentConnectionDescr getCurrentConnectionSettings();
    bool isCurrentIkev2Protocol();
    void saveCurrentSuccessfullConnectionSettings();
    bool isAutomaticMode();

    static void autoTest();

private:

    struct AttemptInfo
    {
        ProtocolType protocol;
        int portMapInd;
        bool changeNode;
    };

    QVector<AttemptInfo> attemps_;
    int curAttempt_;
    int failedIkev2Counter_;
    bool isFailedIkev2CounterAlreadyIncremented_;
    int failedManualModeCounter_;
    static const int MAX_IKEV2_FAILED_ATTEMPTS = 5;

    QSharedPointer<locationsmodel::MutableLocationInfo> mli_;
    ConnectionSettings connectionSettings_;
    apiinfo::PortMap portMap_;
    bool bIsAllFailed_;
    bool bStarted_;

    QString getLogForNode(int ind);
    static bool sortPortMapFunction(const apiinfo::PortItem &p1, const apiinfo::PortItem &p2);
};

#endif // AUTOMANUALCONNECTIONCONTROLLER_H
