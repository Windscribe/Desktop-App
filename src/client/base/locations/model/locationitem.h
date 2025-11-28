#pragma once
#include "types/location.h"

namespace gui_locations {

// Extends the class types::Location with additional functionality for calculating the lowest ping and the location load
// Used by the LocationsModel
class LocationItem
{
public:
    explicit LocationItem(const types::Location &location);
    // construct best location item
    explicit LocationItem(const LocationID &bestLocation, const types::Location &l, int cityInd);

    const types::Location &location() const { return location_; }
    int load();
    int lowestPing();
    QString nickname() const { return nickname_; }
    bool is10gbps() const { return is10gbps_; }

    void insertCityAtInd(int ind, const types::City &city);
    void removeCityAtInd(int ind);
    void updateCityAtInd(int ind, const types::City &city);
    void updateLocation(const types::Location &location);
    void moveCity(int from, int to);
    void setPingTimeForCity(int cityInd, PingTime time);

    void setName(const QString &name);

    bool operator==(const LocationItem &other) const;
    bool operator!=(const LocationItem &other) const;

private:
    types::Location location_;
    int load_;
    int lowestPing_;
    bool bNeedRecalcInternalValue_;

    bool is10gbps_;  // is10gbps makes sense only for the best location
    QString nickname_;  // makes sense only for best location


    void recalcIfNeed();
    void recalcLoad();
    void recalcLowestPing();
};


} //namespace gui_locations

