#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "engine/types/locationid.h"
#include "locationitem.h"

namespace locationsmodel {

// Managing locations. Converts the apiinfo::Location vector to LocationItem vector.
// Add best location, static IPs, custom OVPN-configs location. Updates the ping speed for each location.
// thread safe access to public methods
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent);
    virtual ~LocationsModel();

    void setLocations(const QVector<apiinfo::Location> &locations);
    void setStaticIps(const apiinfo::StaticIps &staticIps);
    // void setCustomOvpnConfigs()
    void clear();

signals:
    void locationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> > items);
    void locationPingTimeChanged(LocationID id, PingTime timeMs);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_H
