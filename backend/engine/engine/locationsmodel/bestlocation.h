#ifndef LOCATIONSMODEL_BESTLOCATION_H
#define LOCATIONSMODEL_BESTLOCATION_H

#include <QString>
#include <QVector>
#include "engine/serversmodel/pingtime.h"

namespace locationsmodel {

class BestLocation
{
public:
    explicit BestLocation();

    bool isValid() const;
    int getId() const;

    void setId(int id);

    void saveToSettings();
    void loadFromSettings();

private:
    bool isValid_;
    int id_;
    int latency_;
};


} //namespace locationsmodel

#endif // LOCATIONSMODEL_BESTLOCATION_H
