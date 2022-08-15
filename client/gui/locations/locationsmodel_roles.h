#ifndef LOCATIONSMODEL_ROLES_H
#define LOCATIONSMODEL_ROLES_H

#include <QObject>

namespace gui_location {

// Roles for LocationsModel items
enum ROLES { LOCATION_ID = Qt::UserRole,    // location and city
             IS_TOP_LEVEL_LOCATION,         // location and city
             INITITAL_INDEX,                // location and city
             COUNTRY_CODE,                  // location and city
             IS_SHOW_P2P,                   // location only
             IS_SHOW_AS_PREMIUM,            // location and city
             IS_10GBPS,                     // location and city
             LOAD,                          // location and city
             NAME,                          // location and city
             NICKNAME,                      // city only and best location
             PING_TIME,                     // location and city (for latency calc average ping)
             IS_FAVORITE,                   // city only

             STATIC_IP_TYPE,                // city only
             STATIC_IP,                     // city only

             IS_DISABLED,                   // city only
             LINK_SPEED,                    // city only

             IS_CUSTOM_CONFIG_CORRECT,      // city only
             CUSTOM_CONFIG_TYPE,            // city only
             CUSTOM_CONFIG_ERROR_MESSAGE   // city only
           };

} //namespace gui_location


#endif // LOCATIONSMODEL_ROLES_H
