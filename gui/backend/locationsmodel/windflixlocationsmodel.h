#ifndef WINDFLIXLOCATIONSMODEL_H
#define WINDFLIXLOCATIONSMODEL_H

#include <QObject>
#include "basiclocationsmodel.h"

class WindflixLocationsModel : public BasicLocationsModel
{
    Q_OBJECT
public:
    explicit WindflixLocationsModel(QObject *parent = nullptr);

    virtual void update(QVector<LocationModelItem *> locations);
};

#endif // WINDFLIXLOCATIONSMODEL_H
