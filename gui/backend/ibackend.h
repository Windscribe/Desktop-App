#ifndef IBACKEND_H
#define IBACKEND_H

#include <QString>
#include "Types/upgrademodetype.h"
#include "LocationsModel/locationsmodel.h"
#include "Preferences/preferenceshelper.h"
#include "Preferences/preferences.h"
#include "Preferences/accountinfo.h"
#include "IPC/generated_proto/types.pb.h"

// abstract interface for backend
class IBackend
{
public:
    virtual ~IBackend() {}

    virtual void init() = 0;
    virtual void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart) = 0;
    virtual void enableBFE_win() = 0;
    virtual void login(const QString &username, const QString &password) = 0;
    virtual bool isCanLoginWithAuthHash() const = 0;
    virtual bool isSavedApiSettingsExists() const = 0;
    virtual void loginWithAuthHash() = 0;
    virtual void loginWithLastLoginSettings() = 0;
    virtual bool isLastLoginWithAuthHash() const = 0;
    virtual void signOut() = 0;
    virtual void connectClick(const LocationID &lid) = 0;
    virtual void disconnectClick() = 0;
    virtual bool isDisconnected() const = 0;
    virtual ProtoTypes::ConnectStateType currentConnectState() const = 0;

    virtual void firewallOn() = 0;
    virtual void firewallOff() = 0;
    virtual bool isFirewallEnabled() = 0;

    virtual void startWifiSharing(const QString &ssid, const QString &password) = 0;
    virtual void stopWifiSharing() = 0;
    virtual void startProxySharing(ProtoTypes::ProxySharingMode proxySharingMode) = 0;
    virtual void stopProxySharing() = 0;

    virtual void recordInstall() = 0;
    virtual void sendConfirmEmail() = 0;
    virtual void sendDebugLog() = 0;

    virtual void setBlockConnect(bool isBlockConnect) = 0;
    virtual void clearCredentials() = 0;

    virtual bool isInitFinished() = 0;
    virtual bool isAppCanClose() = 0;

    virtual void sendEngineSettingsIfChanged() = 0;

    virtual LocationsModel *getLocationsModel() = 0;

    virtual PreferencesHelper *getPreferencesHelper() = 0;
    virtual Preferences *getPreferences() = 0;
    virtual AccountInfo *getAccountInfo() = 0;

    virtual void applicationActivated() = 0;
    virtual void applicationDeactivated() = 0;

    virtual void setShareSecureHotspot(const ProtoTypes::ShareSecureHotspot &ss) = 0;
    virtual const ProtoTypes::ShareSecureHotspot &getShareSecureHotspot() const = 0;

    virtual void setShareProxyGateway(const ProtoTypes::ShareProxyGateway &sp) = 0;
    virtual const ProtoTypes::ShareProxyGateway &getShareProxyGateway() const = 0;

    virtual const ProtoTypes::SessionStatus &getSessionStatus() const = 0;

    virtual void handleNetworkChange(ProtoTypes::NetworkInterface networkInterface) = 0;

    virtual void cycleMacAddress() = 0;

signals:
    virtual void initFinished(ProtoTypes::InitState initState) = 0;
    virtual void cleanupFinished() = 0;

    virtual void loginInfoChanged(const ProtoTypes::LoginInfo &loginInfo) = 0;
    virtual void myIpChanged(QString ip, bool isFromDisconnectedState) = 0;
    virtual void connectStateChanged(const ProtoTypes::ConnectState &connectState) = 0;
    virtual void firewallStateChanged(bool isEnabled) = 0;
    virtual void notificationsChanged(const ProtoTypes::ArrayApiNotification &arr) = 0;
    virtual void networkChanged(ProtoTypes::NetworkInterface interface) = 0;
    virtual void sessionStatusChanged(const ProtoTypes::SessionStatus &sessionStatus) = 0;
    virtual void checkUpdateChanged(const ProtoTypes::CheckUpdateInfo &checkUpdateInfo) = 0;

    virtual void confirmEmailResult(bool bSuccess) = 0;
    virtual void debugLogResult(bool bSuccess) = 0;
    virtual void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes) = 0;


    virtual void shareSecureHotspotChanged(const ProtoTypes::ShareSecureHotspot &ss) = 0;
    virtual void shareProxyGatewayChanged(const ProtoTypes::ShareProxyGateway &sp) = 0;

};

#endif // IBACKEND_H
