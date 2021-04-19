#ifndef CUSTOMCONFIGLOCATIONINFO_H
#define CUSTOMCONFIGLOCATIONINFO_H

#include <QHostInfo>
#include <QSharedPointer>
#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"
#include "engine/types/wireguardconfig.h"

namespace locationsmodel {

// describe consig location
class CustomConfigLocationInfo : public BaseLocationInfo
{
    Q_OBJECT
public:
    explicit CustomConfigLocationInfo(const LocationID &locationId,
                                      QSharedPointer<const customconfigs::ICustomConfig> config);

    bool isExistSelectedNode() const override;
    QString getLogString() const override;

    void resolveHostnames();

    // ovpn-specific
    QString getSelectedIp() const;
    QString getSelectedRemoteCommand() const;
    uint getSelectedPort() const;
    QString getSelectedProtocol() const;
    QString getOvpnData() const;
    QString getFilename() const;
    QSharedPointer<WireGuardConfig> getWireguardCustomConfig(const QString &endpointIp) const;
    bool isAllowFirewallAfterConnection() const;
    void selectNextNode();


signals:
    void hostnamesResolved();

private slots:
    void onResolved(const QString &hostname, const QStringList &ips, void *userPointer);

private:
    struct RemoteDescr
    {
        QString ipOrHostname_;
        bool isHostname;
        bool isResolved;
        QStringList ipsForHostname_;

        QString remoteCmdLine;
        uint port;              // 0 if not set
        QString protocol;       // empty if not set
    };

    void resolveHostnamesForWireGuardConfig();
    void resolveHostnamesForOVPNConfig();

    QSharedPointer<const customconfigs::ICustomConfig> config_;
    QVector<RemoteDescr> remotes_;
    QString globalProtocol_;
    uint globalPort_;

    bool bAllResolved_;
    int selected_;          // index in remotes_ array
    int selectedHostname_;  // index in remotes_[selected].ipsForHostname, or 0 if
                            // remotes_[selected].isHostname == false

    bool isAllResolved() const;

};

} //namespace locationsmodel

#endif // CUSTOMCONFIGLOCATIONINFO_H
