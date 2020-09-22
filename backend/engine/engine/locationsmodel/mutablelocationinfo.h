#ifndef MUTABLELOCATIONINFO_H
#define MUTABLELOCATIONINFO_H

#include <QObject>
#include <QVector>

#include "engine/types/locationid.h"
#include "locationnode.h"
#include "engine/apiinfo/staticips.h"


namespace locationsmodel {

// describe location info (nodes, selected item, iterate over nodes)
// the location info can automatically changed (new nodes added, nodes ips changed, etc) if this location changed in LocationsModel
// even when the location info changes, the possibility of iteration over nodes remains correct
class MutableLocationInfo : public QObject
{
    Q_OBJECT
public:
    explicit MutableLocationInfo(const LocationID &locationId, const QString &name,
                                 const QVector< QSharedPointer<BaseNode> > &nodes, int selectedNode,
                                 const QString &dnsHostName);


    bool isCustomOvpnConfig() const;
    bool isStaticIp() const;
    QString getName() const;
    QString getDnsName() const;
    QString getCustomOvpnConfigPath() const;
    bool isExistSelectedNode() const;
    int nodesCount() const;
    QString getIpForNode(int indNode, int indIp) const;

    void selectNextNode();

    QString getIpForSelectedNode(int indIp) const;
    QString getHostnameForSelectedNode() const;
    QString getWgPubKeyForSelectedNode() const;

    QString getStaticIpUsername() const;
    QString getStaticIpPassword() const;
    apiinfo::StaticIpPortsVector getStaticIpPorts() const;

    // need to do for custom ovpn config, if hostname specified in ovpn-file
    /*QString resolveHostName();*/

public slots:
    //void locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);

private:
    LocationID locationId_;
    QString name_;
    QVector< QSharedPointer<BaseNode> > nodes_;
    int selectedNode_;
    QString dnsHostName_;
};

} //namespace locationsmodel

#endif // MUTABLELOCATIONINFO_H
