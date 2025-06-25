#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "scapix_object.h"
#include "WSNetCancelableCallback.h"
#include "WSNetServerAPI.h"

namespace wsnet {

enum class ApiResourcesManagerNotification {
    kLoginOk = 0,     // login was successful and API resources (SessionStatus, Locations, ServerCredentials(both openvpn and ikev2), ServerConfigs, PortMap, StatisIPS) are ready for use
    kLoginFailed,     // login failed, returns error code in LoginResult variable and error message in string
    kSessionUpdated,  // SessionStatus
    kSessionDeleted,
    kServerCredentialsUpdated,  // ServerCredentials(both openvpn and ikev2), ServerConfigs
    kLocationsUpdated,
    kStaticIpsUpdated,
    kNotificationsUpdated,
    kCheckUpdate,
    kLogoutFinished,
    kAuthTokenLoginFinished
};

enum class LoginResult {
    kSuccess = 0,
    kNoApiConnectivity,
    kNoConnectivity,
    kIncorrectJson,
    kBadUsername,
    kSslError,
    kBadCode2fa,
    kMissingCode2fa,
    kAccountDisabled,
    kSessionInvalid,
    kRateLimited,
    kInvalidSecurityToken
};

typedef std::function<void(ApiResourcesManagerNotification notification, LoginResult loginResult, const std::string &errorMessage)> WSNetApiResourcesManagerCallback;

// The class provides management of API resources such as SessionStatus, Locations, ServerCredentials,
// ServerConfigs, PortMap, StatisIPS, Notifications, CheckUpdate.
// Also stores the last received resources in a persistent storage.
// Also ensures that resources are updated according to the schedule and notifies the client of this via callback functions.
// When using this class in the client, you should not call the following functions from WSNetServerAPI:
// login, session, deleteSession, serverLocations, serverCredentials, serverConfigs, portMap, checkUpdate, staticIps, notifications.
class WSNetApiResourcesManager : public scapix_object<WSNetApiResourcesManager>
{
public:
    virtual ~WSNetApiResourcesManager() {}

    // callback function for notifications
    virtual std::shared_ptr<WSNetCancelableCallback> setCallback(WSNetApiResourcesManagerCallback callback) = 0;

    // Set an existing authHash. Made so that when updating from older versions of the program, where the hash is stored in other settings, the user does not need to log in again.
    virtual void setAuthHash(const std::string &authHash) = 0;

    // Is there all the API data in a persistent repository?
    // They can be from the last time the program was run or they can be fresh.
    // If they are present, the client is ready to work and switch to the connect screen.
    virtual bool isExist() const = 0;

    // login with saved session authHash, return false if no saved authHash, true otherwise.
    // Returns the result in the notifications kLoginOk/kLoginFailed
    virtual bool loginWithAuthHash() = 0;

    // authTokenLogin call, must be called before login API call. Required for two stage login + CAPTCHA
    virtual void authTokenLogin(bool useAsciiCaptcha) = 0;

    // login with username/password
    // optionally send captcha data
    // Returns the result in the notifications kLoginOk/kLoginFailed
    virtual void login(const std::string &username, const std::string &password, const std::string &code2fa,
                       const std::string &secureToken,
                       const std::string &captchaSolution = std::string(),
                       const std::vector<float> &captchaTrailX = std::vector<float>(),
                       const std::vector<float> &captchaTrailY = std::vector<float>()) = 0;

    // does session deletion and deletes all saved data from persistent storage
    virtual void logout() = 0;

    // force update session right now without regard to scheduling
    virtual void fetchSession() = 0;

    // force update the server credentials(openvpn and ikev2) and openvpn config right now without regard to scheduling
    // Notify with kServerCredentialsUpdated code.
    virtual void fetchServerCredentials() = 0;

    // return current authHash or empty string, if no authHash
    virtual std::string authHash() = 0;
    // remove all saved API data from persistent settings
    virtual void removeFromPersistentSettings() = 0;

    // makes an immediate call to check update and runs a regular update every 24 hours
    virtual void checkUpdate(UpdateChannel channel, const std::string &appVersion, const std::string &appBuild,
                             const std::string &osVersion, const std::string &osBuild) = 0;

    // pcpid used for Notifications API call
    virtual void setNotificationPcpid(const std::string &pcpid) = 0;

    // appleId or gpDeviceId (ios or android) device ID, set them empty if they are not required
    virtual void setMobileDeviceId(const std::string &appleId, const std::string &gpDeviceId) = 0;

    // the following functions return the current API data in json format
    virtual std::string sessionStatus() const = 0;
    virtual std::string portMap() const = 0;
    virtual std::string locations() const = 0;
    virtual std::string staticIps() const = 0;
    virtual std::string serverCredentialsOvpn() const = 0;
    virtual std::string serverCredentialsIkev2() const = 0;
    virtual std::string serverConfigs() const = 0;
    virtual std::string notifications() const = 0;
    virtual std::string checkUpdate() const = 0;
    virtual std::string authTokenLoginResult() const = 0;

    // this function is for debugging purposes, allows to set arbitrary resource update intervals
    virtual void setUpdateIntervals(int sessionInDisconnectedStateMs, int sessionInConnectedStateMs,
                                    int locationsMs, int staticIpsMs, int serverConfigsAndCredentialsMs,
                                    int portMapMs, int notificationsMs, int checkUpdateMs) = 0;

};

} // namespace wsnet
