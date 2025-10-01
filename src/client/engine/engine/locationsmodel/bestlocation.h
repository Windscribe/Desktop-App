#pragma once

#include <QString>
#include <QVector>
#include "types/pingtime.h"
#include "types/locationid.h"

namespace locationsmodel {

// best location info, id persistent between runs of the program
class BestLocation
{
public:
    explicit BestLocation();
    virtual ~BestLocation();

    bool isValid() const;
    LocationID getId() const;

    void set(const LocationID &id, bool isDetectedFromThisAppStart, bool isDetectedWithDisconnectedIps);

    bool isDetectedFromThisAppStart() const;
    bool isDetectedWithDisconnectedIps() const;

    void saveToSettings();
    void loadFromSettings();

private:
    bool isValid_;
    bool isDetectedFromThisAppStart_;        // if the location is detected with pings with this app start (not readed from settings or detected as random first)
    bool isDetectedWithDisconnectedIps_;     // if the location is detected without pings in connected state
    LocationID id_;

    // for serialization
    static constexpr quint32 magic_ = 0x77AACBAE;
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace locationsmodel
