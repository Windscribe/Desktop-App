#pragma once

#include <QObject>

namespace gui_locations {

// Roles for LocationsModel items
enum ROLES { LOCATION_ID = Qt::UserRole,    // location and city
             IS_TOP_LEVEL_LOCATION,         // true for a location and false for a city
             COUNTRY_CODE,                  // location and city
             IS_SHOW_P2P,                   // location only
             IS_SHOW_AS_PREMIUM,            // location and city
             IS_10GBPS,                     // location and city
             LOAD,                          // location and city
             NAME,                          // location and city
             NICKNAME,                      // city only and best location
             PING_TIME,                     // location and city (for latency calc average ping)
             IS_FAVORITE,                   // city only
             IS_DISABLED,                   // city and location, the location is disabled if it has no cities

             STATIC_IP_TYPE,                // city only
             STATIC_IP,                     // city only

             IS_CUSTOM_CONFIG_CORRECT,      // city only
             CUSTOM_CONFIG_TYPE,            // city only
             CUSTOM_CONFIG_ERROR_MESSAGE   // city only
           };

} //namespace gui_locations

