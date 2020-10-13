#ifndef STATICIPSCITIESMODEL_H
#define STATICIPSCITIESMODEL_H

#include "basiccitiesmodel.h"

class StaticIpsCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit StaticIpsCitiesModel(QObject *parent = nullptr);

    void update(QVector<QSharedPointer<LocationModelItem> > locations) override;

};

#endif // STATICIPSCITIESMODEL_H
