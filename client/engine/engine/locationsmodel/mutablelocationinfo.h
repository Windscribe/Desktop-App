#pragma once

#include <QObject>
#include <QVector>

#include "locationnode.h"
#include "baselocationinfo.h"

namespace locationsmodel {

// describe location info (nodes, selected item, iterate over nodes)
// the location info can be automatically changed (new nodes added, nodes ips changed, etc) if this location changed in LocationsModel
// even when the location info changes, the possibility of iteration over nodes remains correct
class MutableLocationInfo : public BaseLocationInfo
{
    Q_OBJECT
public:
    explicit MutableLocationInfo(const LocationID &locationId, const QString &name,
                                 const QVector< QSharedPointer<const BaseNode> > &nodes, int selectedNode,
                                 const QString &dnsHostName, const QString &verifyX509name);


    QString getDnsName() const;
    bool isExistSelectedNode() const override;
    QString getVerifyX509name() const override;
    QString getLogString() const override;

    int nodesCount() const;
    QString getIpForNode(int indNode, int indIp) const;

    void selectNextNode();
    void selectNodeByIp(const QString &addr);

    QString getIpForSelectedNode(int indIp) const;
    QString getHostnameForSelectedNode() const;
    QString getWgPubKeyForSelectedNode() const;
    QString getWgIpForSelectedNode() const;

    QString getStaticIpUsername() const;
    QString getStaticIpPassword() const;
    api_responses::StaticIpPortsVector getStaticIpPorts() const;

public slots:
    //void locationChanged(const LocationID &locationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);

private:
    QVector< QSharedPointer<const BaseNode> > nodes_;
    int selectedNode_;
    QString dnsHostName_;
    QString verifyX509name_;

    QString getLogForNode(int ind) const;

};

} //namespace locationsmodel
