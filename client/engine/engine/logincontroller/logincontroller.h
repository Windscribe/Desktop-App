#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QElapsedTimer>
#include <QObject>
#include "engine/serverapi/serverapi.h"
#include "getallconfigscontroller.h"
#include "getapiaccessips.h"
#include "types/dnsresolutionsettings.h"
#include "types/loginsettings.h"
#include "engine/apiinfo/apiinfo.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"

//class IFirewallController;
class ServerAPI;
class IHelper;

// manage process from login to goto connection screen (when we show waiting screen) on application start,
// set API ip/domain to serverAPI if initialization success
class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(QObject *parent, IHelper *helper, INetworkDetectionManager *networkDetectionManager, ServerAPI *serverAPI,
                             const QString &language, PROTOCOL protocol);
    virtual ~LoginController();

    void startLoginProcess(const types::LoginSettings &loginSettings, const types::DnsResolutionSettings &dnsResolutionSettings, bool bFromConnectedToVPNState);

signals:
    void readyForNetworkRequests();
    void stepMessage(LOGIN_MESSAGE msg);
    void finished(LOGIN_RET retCode, const apiinfo::ApiInfo &apiInfo, bool bFromConnectedToVPNState, const QString &errorMessage);

private slots:
    void onLoginAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, const QString &authHash, uint userRole, const QString &errorMessage);
    void onSessionAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, uint userRole);
    void onServerLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<types::Location> &serverLocations, QStringList forceDisconnectNodes, uint userRole);
    void onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword, PROTOCOL protocol, uint userRole);
    void onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole);
    void onPortMapAnswer(SERVER_API_RET_CODE retCode, const types::PortMap &portMap, uint userRole);
    void onStaticIpsAnswer(SERVER_API_RET_CODE retCode, const types::StaticIps &staticIps, uint userRole);

    void onGetApiAccessIpsFinished(SERVER_API_RET_CODE retCode, const QStringList &hosts);

    void tryLoginAgain();
    void onAllConfigsReceived(SERVER_API_RET_CODE retCode);
    void getAllConfigs();

    void handleNetworkConnection();

private:
    enum {MAX_WAIT_CONNECTIVITY_TIMEOUT = 20000};
    enum {MAX_WAIT_LOGIN_TIMEOUT = 10000};

    enum LOGIN_STEP {LOGIN_STEP1 = 0, LOGIN_STEP2, LOGIN_STEP3};

    QVector<SERVER_API_RET_CODE> retCodesForLoginSteps_;

    IHelper *helper_;
    ServerAPI *serverAPI_;

    uint serverApiUserRole_;
    GetApiAccessIps *getApiAccessIps_;
    INetworkDetectionManager *networkDetectionManager_;
    QString language_;
    PROTOCOL protocol_;

    types::LoginSettings loginSettings_;
    QString newAuthHash_;

    types::DnsResolutionSettings dnsResolutionSettings_;
    bool bFromConnectedToVPNState_;

    QElapsedTimer loginElapsedTimer_;
    QElapsedTimer waitNetworkConnectivityElapsedTimer_;

    types::SessionStatus sessionStatus_;

    GetAllConfigsController *getAllConfigsController_;
    LOGIN_STEP loginStep_;
    bool readyForNetworkRequestsEmitted_;

    QStringList ipsForStep3_;

    void getApiInfoFromSettings();
    void handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, const QString &authHash, const QString &errorMessage);
    void makeLoginRequest(const QString &hostname);
    void makeApiAccessRequest();
    QString selectRandomIpForStep3();

    bool isAllSslErrors() const;
    void handleNextLoginAfterFail(SERVER_API_RET_CODE retCode);
};

#endif // LOGINCONTROLLER_H
