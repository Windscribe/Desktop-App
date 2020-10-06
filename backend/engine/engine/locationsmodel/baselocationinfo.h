#ifndef BASELOCATIONINFO_H
#define BASELOCATIONINFO_H

#include <QObject>

#include "types/locationid.h"


namespace locationsmodel {

// describe location info (nodes, selected item, iterate over nodes, custom config, static IP)
// the location info can be automatically changed (new nodes added, nodes ips changed, etc) if this location changed in LocationsModel
// even when the location info changes, the possibility of iteration over nodes remains correct

class BaseLocationInfo : public QObject
{
    Q_OBJECT
public:
    explicit BaseLocationInfo(const LocationID &locationId, const QString &name);
    virtual ~BaseLocationInfo() {}

    const LocationID &locationId() const;
    QString getName() const;
    virtual bool isExistSelectedNode() const = 0;

protected:
    LocationID locationId_;
    QString name_;
};

} //namespace locationsmodel

#endif // BASELOCATIONINFO_H
