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

    void update(QVector<LocationModelItem *> locations) override;
};

#endif // ALLLOCATIONSMODEL_H
