#pragma once

#include <QObject>

namespace gui_locations {

// Roles for LocationsModel items
enum Roles {
     kLocationId = Qt::UserRole,          // location and city
     kIsTopLevelLocation,                 // true for a location and false for a city
     kCountryCode,                        // location and city
     kIsShowP2P,                          // location only
     kIsShowAsPremium,                    // location and city
     kIs10Gbps,                           // location and city
     kLoad,                               // location and city
     kName,                               // location and city
     kNick,                               // city only and best location
     kPingTime,                           // location and city (for latency calc average ping)
     kIsFavorite,                         // city only
     kIsDisabled,                         // city and location, the location is disabled if it has no cities

     kStaticIpType,                       // city only
     kStaticIp,                           // city only

     kIsCustomConfigCorrect,              // city only
     kCustomConfigType,                   // city only
     kCustomConfigErrorMessage            // city only
};

} //namespace gui_locations

