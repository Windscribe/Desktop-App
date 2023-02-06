#ifndef PARAMETERS_H
#define PARAMETERS_H

// the parameter that is passed to the function d1(...)  which determines the type of secret to be returned
// more details: https://www.notion.so/windscribe/Client-Control-Plane-Failover-e4a5a0d1cf1045c99a9789db782fb19e#50ff141584b540a0b1334c78221c15dc

#define PAR_LIBRARY_VERSION             0       // integer version number of this library in the string, for example "1", "2", ...

#define PAR_API_ACCESS_IP1              1       // 138.197.150.76
#define PAR_API_ACCESS_IP2              2       // 139.162.150.150

#define PAR_BACKUP_DOMAIN               100     // totallyacdn.com

#define PAR_RANDOM_GENERATED_DOMAIN     200     // current DGA domain based on date and unique hardware id

#define PAR_DYNAMIC_DOMAIN_URL1         300     // https://1.1.1.1/dns-query
#define PAR_DYNAMIC_DOMAIN_URL2         301     // https://dns.google/resolve

#define PAR_DYNAMIC_DOMAIN_DESKTOP      400     // dynamic-api-host2.windscribe.com
#define PAR_DYNAMIC_DOMAIN_MOBILE       401     // dynamic-api-host3.windscribe.com

#define PAR_EMERGENCY_IP1               500     // 138.197.162.195
#define PAR_EMERGENCY_IP2               501     // 104.131.166.124

#define PAR_EMERGENCY_USERNAME          600     // windscribe
#define PAR_EMERGENCY_PASSWORD          601     // Xeo6kYR2

#define PAR_PASSWORD_FOR_RANDOM_DOMAIN  700     // giveMEsomePACKETZ9! (deprecated)

#endif // PARAMETERS_H
