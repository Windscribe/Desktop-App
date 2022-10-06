#pragma once

#include <QElapsedTimer>
#include <QObject>
#include "engine/serverapi/requests/sessionerrorcode.h"
#include "engine/serverapi/serverapi.h"
#include "getallconfigscontroller.h"
#include "loginsettings.h"
#include "engine/apiinfo/apiinfo.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"

// manage process from login to goto connection screen (when we show waiting screen) on application start,
// in other words, it receives and fills in all the data in apiinfo::ApiInfo
class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(QObject *parent, INetworkDetectionManager *networkDetectionManager, server_api::ServerAPI *serverAPI,
                             const QString &language, PROTOCOL protocol);

    void startLoginProcess(const LoginSettings &loginSettings, bool bFromConnectedToVPNState);

signals:
    void finished(LOGIN_RET retCode, const apiinfo::ApiInfo &apiInfo, bool bFromConnectedToVPNState, const QString &errorMessage);

private slots:
    void onLoginAnswer();
    void onSessionAnswer();
    void onServerLocationsAnswer();
    void onServerCredentialsAnswer();
    void onServerConfigsAnswer();
    void onPortMapAnswer();
    void onStaticIpsAnswer();

    void tryLoginAgain();
    void onAllConfigsReceived(SERVER_API_RET_CODE retCode);
    void getAllConfigs();

    void handleNetworkConnection();

private:
    enum {MAX_WAIT_CONNECTIVITY_TIMEOUT = 20000};

    server_api::ServerAPI *serverAPI_;

    INetworkDetectionManager *networkDetectionManager_;
    QString language_;
    PROTOCOL protocol_;

    LoginSettings loginSettings_;
    QString newAuthHash_;

    bool bFromConnectedToVPNState_;

    QElapsedTimer loginElapsedTimer_;
    QElapsedTimer waitNetworkConnectivityElapsedTimer_;

    types::SessionStatus sessionStatus_;

    GetAllConfigsController *getAllConfigsController_;

    void getApiInfoFromSettings();
    void handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, server_api::SessionErrorCode sessionErrorCode,  const types::SessionStatus &sessionStatus, const QString &authHash, const QString &errorMessage);
    void makeLoginRequest();

    void handleNextLoginAfterFail(SERVER_API_RET_CODE retCode);
};
