#pragma once

#include "scapix_object.h"

#include "exported.h"
#include "WSNetDnsResolver.h"
#include "WSNetHttpNetworkManager.h"
#include "WSNetServerAPI.h"
#include "WSNetEmergencyConnect.h"
#include "WSNetPingManager.h"
#include "WSNetAdvancedParameters.h"
#include "WSNetApiResourcesManager.h"
#include "WSNetDecoyTraffic.h"
#include "WSNetUtils.h"

namespace wsnet {

typedef std::function<void(const std::string &)> WSNetLoggerFunction;

class EXPORTED WSNet : public scapix_object<WSNet>
{
public:
    virtual ~WSNet() {}

    // set debugLog to true for more verbose log output
    // call this function only once before initialize the library
    static void setLogger(WSNetLoggerFunction loggerFunction, bool debugLog);

    // basePlatform value can be "windows", "mac", "linux", "android", "ios"
    // platformName and appVersion values are added to each request.
    // difference between basePlatform and platformName is that platformName is more specific (for example windows_arm64/windows).
    // deviceId - unique device identifier, in particular used for the API StaticIps
    // must supply sessionTypeId, where 3 = DESKTOP, 4 = MOBILE (ios and android) to get an appropriate session type token

    static bool initialize(const std::string &basePlatform,  const std::string &platformName, const std::string &appVersion,
                           const std::string &deviceId, const std::string &openVpnVersion, const std::string &sessionTypeId,
                           bool isUseStagingDomains, const std::string &language, const std::string &persistentSettings);
    static std::shared_ptr<WSNet> instance();
    static void cleanup();
    static bool isValid();

    virtual void setConnectivityState(bool isOnline) = 0;
    virtual void setIsConnectedToVpnState(bool isConnected) = 0;

    virtual std::string currentPersistentSettings() = 0;

    virtual std::shared_ptr<WSNetDnsResolver> dnsResolver() = 0;
    virtual std::shared_ptr<WSNetHttpNetworkManager> httpNetworkManager() = 0;
    virtual std::shared_ptr<WSNetServerAPI> serverAPI() = 0;
    virtual std::shared_ptr<WSNetApiResourcesManager> apiResourcersManager() = 0;
    virtual std::shared_ptr<WSNetEmergencyConnect> emergencyConnect() = 0;
    virtual std::shared_ptr<WSNetPingManager> pingManager() = 0;
    virtual std::shared_ptr<WSNetDecoyTraffic> decoyTraffic() = 0;
    virtual std::shared_ptr<WSNetAdvancedParameters> advancedParameters() = 0;
    virtual std::shared_ptr<WSNetUtils> utils() = 0;
};

} // namespace wsnet
