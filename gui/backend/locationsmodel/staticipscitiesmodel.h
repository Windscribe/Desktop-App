#ifndef STATICIPSCITIESMODEL_H
#define STATICIPSCITIESMODEL_H

#include "basiccitiesmodel.h"

class StaticIpsCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit StaticIpsCitiesModel(QObject *parent = nullptr);

    virtual void update(QVector<LocationModelItem *> locations);

};

#endif // STATICIPSCITIESMODEL_H
