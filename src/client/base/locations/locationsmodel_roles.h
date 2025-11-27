#pragma once

#include <QObject>

namespace gui_locations {

// Roles for LocationsModel items
enum Roles {
    kLocationId = Qt::UserRole,          // location and city
    kIsTopLevelLocation,                 // true for a location and false for a city
    kCountryCode,                        // location and city
    kShortName,                          // location and city
    kIsShowP2P,                          // location only
    kIsShowAsPremium,                    // location and city
    kIs10Gbps,                           // location and city
    kLoad,                               // location and city
    kName,                               // location and city
    kNick,                               // city only and best location
    kPingTime,                           // location and city (for location: lowest ping of all cities)
    kIsFavorite,                         // city only
    kPinnedIp,                           // city only
    kIsDisabled,                         // city and location, the location is disabled if it has no cities

    kStaticIpType,                       // city only
    kStaticIp,                           // city only

    kIsCustomConfigCorrect,              // city only
    kCustomConfigType,                   // city only
    kCustomConfigErrorMessage,           // city only

    kDisplayNickname,                    // city only and best location
};

} //namespace gui_locations

