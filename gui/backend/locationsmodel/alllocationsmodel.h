#ifndef ALLLOCATIONSMODEL_H
#define ALLLOCATIONSMODEL_H

#include <QObject>
#include "basiclocationsmodel.h"

// model of locations with cities
class AllLocationsModel : public BasicLocationsModel
{
    Q_OBJECT
public:
    explicit AllLocationsModel(QObject *parent = nullptr);

    virtual void update(QVector<LocationModelItem *> locations);
};

#endif // ALLLOCATIONSMODEL_H
