#ifndef MUTABLELOCATIONINFO_H
#define MUTABLELOCATIONINFO_H

#include <QObject>
#include <QVector>

#include "engine/types/locationid.h"
#include "engine/types/servernode.h"

// describe location info (nodes, selected item, iterate over nodes)
// the location info can automatically changed (new nodes added, nodes ips changed, etc) if this location changed in ServersModel
// even when the location info changes, the possibility of iteration over nodes remains correct
class MutableLocationInfo : public QObject
{
    Q_OBJECT
public:
    explicit MutableLocationInfo(const LocationID &locationId, const QString &name,
                                 /*const QVector<ServerNode> &nodes,*/ int selectedNode,
                                 const QString &dnsHostName);
    virtual ~MutableLocationInfo();

    bool isCustomOvpnConfig() const;
    bool isStaticIp() const;
    QString getName() const;
    QString getDnsName() const;
    QString getCustomOvpnConfigPath() const;
    int isExistSelectedNode() const;
    //const ServerNode &getSelectedNode() const;
    int nodesCount() const;
    //const ServerNode &getNode(int ind) const;

    void selectNextNode();

    QString getStaticIpUsername() const;
    QString getStaticIpPassword() const;
    StaticIpPortsVector getStaticIpPorts() const;

    // need to do for custom ovpn config, if hostname specified in ovpn-file
    QString resolveHostName();

public slots:
    void locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);

private:
    Qt::HANDLE threadHandle_;
    LocationID locationId_;
    QString name_;
    //QVector<ServerNode> nodes_;
    int selectedNode_;
    QString dnsHostName_;
};

#endif // MUTABLELOCATIONINFO_H
