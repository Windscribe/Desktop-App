#ifndef CUSTOMCONFIGLOCATIONINFO_H
#define CUSTOMCONFIGLOCATIONINFO_H

#include <QSharedPointer>
#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"


namespace locationsmodel {

// describe consig location
class CustomConfigLocationInfo : public BaseLocationInfo
{
    Q_OBJECT
public:
    explicit CustomConfigLocationInfo(const LocationID &locationId, const QString &name, QSharedPointer<const customconfigs::ICustomConfig> config);

private:
    QSharedPointer<const customconfigs::ICustomConfig> config_;
};

} //namespace locationsmodel

#endif // CUSTOMCONFIGLOCATIONINFO_H
