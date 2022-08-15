#ifndef GUI_LOCATIONS_MODEL_LOCATIONITEM_WRAPPER_H
#define GUI_LOCATIONS_MODEL_LOCATIONITEM_WRAPPER_H

#include "types/locationitem.h"

namespace gui_location {


// Extends the class types::LocationItem with additional functionality for calculating the average ping and the location load
// May also represent a best location
class LocationItemWrapper
{
public:
    explicit LocationItemWrapper(const types::LocationItem &location);
    // constructor for make the best location
    explicit LocationItemWrapper(const LocationID &bestLocation, const types::LocationItem &l, int cityInd);

    int getLoad() const;
    int getAveragePing() const;
    bool is10Gbps() const;
    QString nickname() const;

    bool operator==(const LocationItemWrapper &other) const;
    bool operator!=(const LocationItemWrapper &other) const;

    // utils functions
    static QVector<int> findRemovedLocations(const QVector<LocationItemWrapper> &original, const QVector<LocationItemWrapper> &changed);
    static QVector<int> findNewLocations(const QVector<LocationItemWrapper> &original, const QVector<LocationItemWrapper> &changed);
    static QVector<int> findChangedLocations(const QVector<LocationItemWrapper> &original, const QVector<LocationItemWrapper> &changed);

private:
    types::LocationItem location_;
    int load_;
    int averagePing_;
    bool is10gbps_;  // is10gbps makes sense only for the best location

    void recalcLoad();
    void recalcAveragePing();
};




} //namespace gui_location

#endif // GUI_LOCATIONS_MODEL_LOCATIONITEM_WRAPPER_H
