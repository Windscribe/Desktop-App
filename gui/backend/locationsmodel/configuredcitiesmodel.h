#ifndef CONFIGUREDCITIESMODEL_H
#define CONFIGUREDCITIESMODEL_H

#include "basiccitiesmodel.h"

class ConfiguredCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit ConfiguredCitiesModel(QObject *parent = nullptr);

    virtual void update(QVector<LocationModelItem *> locations);
};

#endif // CONFIGUREDCITIESMODEL_H
