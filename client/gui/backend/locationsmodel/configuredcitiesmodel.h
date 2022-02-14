#ifndef CONFIGUREDCITIESMODEL_H
#define CONFIGUREDCITIESMODEL_H

#include "basiccitiesmodel.h"

class ConfiguredCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit ConfiguredCitiesModel(QObject *parent = nullptr);

    void update(QVector<QSharedPointer<LocationModelItem> > locations) override;

protected:
    void sort(QVector<CityModelItem *> &cities) override;
};

#endif // CONFIGUREDCITIESMODEL_H
