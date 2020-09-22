#ifndef LOCATIONSMODEL_BESTLOCATION_H
#define LOCATIONSMODEL_BESTLOCATION_H

#include <QString>
#include <QVector>
#include "engine/serversmodel/pingtime.h"

namespace locationsmodel {

// best location info, id persistent between runs of the program
class BestLocation
{
public:
    explicit BestLocation();
    virtual ~BestLocation();

    bool isValid() const;
    int getId() const;

    void set(int id, bool isDetectedFromThisAppStart, bool isDetectedWithDisconnectedIps);

    bool isDetectedFromThisAppStart() const;
    bool isDetectedWithDisconnectedIps() const;

    void saveToSettings();
    void loadFromSettings();

private:
    bool isValid_;
    bool isDetectedFromThisAppStart_;        // if the location is detected with pings with this app start (not readed from settings or detected as random first)
    bool isDetectedWithDisconnectedIps_;     // if the location is detected without pings in connected state
    int id_;
};


} //namespace locationsmodel

#endif // LOCATIONSMODEL_BESTLOCATION_H
