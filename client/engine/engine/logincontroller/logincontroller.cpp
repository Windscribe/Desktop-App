#include "utils/logger.h"
#include <QTimer>
#include "logincontroller.h"
#include "engine/serverapi/serverapi.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "engine/getdeviceid.h"
#include "engine/serverapi/requests/loginrequest.h"
#include "engine/serverapi/requests/sessionrequest.h"
#include "engine/serverapi/requests/serverlistrequest.h"
#include "engine/serverapi/requests/servercredentialsrequest.h"
#include "engine/serverapi/requests/serverconfigsrequest.h"
#include "engine/serverapi/requests/portmaprequest.h"
#include "engine/serverapi/requests/staticipsrequest.h"

LoginController::LoginController(QObject *parent,
                                 INetworkDetectionManager *networkDetectionManager, server_api::ServerAPI *serverAPI,
                                 const QString &language, PROTOCOL protocol) : QObject(parent),
    serverAPI_(serverAPI),
    networkDetectionManager_(networkDetectionManager), language_(language),
    protocol_(protocol), bFromConnectedToVPNState_(false), getAllConfigsController_(NULL)
{
}

void LoginController::startLoginProcess(const LoginSettings &loginSettings, bool bFromConnectedToVPNState)
{
    if (loginSettings.isAuthHashLogin())
    {
         qCDebug(LOG_BASIC) << "Start login process with authHash, bFromConnectedToVPNState =" << bFromConnectedToVPNState;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Start login process with username and password, bFromConnectedToVPNState =" << bFromConnectedToVPNState;
    }

    loginSettings_ = loginSettings;
    bFromConnectedToVPNState_ = bFromConnectedToVPNState;
    handleNetworkConnection();
}

void LoginController::onLoginAnswer()
{
    QSharedPointer<server_api::LoginRequest> request(static_cast<server_api::LoginRequest *>(sender()), &QObject::deleteLater);
    handleLoginOrSessionAnswer(request->networkRetCode(), request->sessionErrorCode(), request->sessionStatus(), request->authHash(), request->errorMessage());
}

void LoginController::onSessionAnswer()
{
    QSharedPointer<server_api::SessionRequest> request(static_cast<server_api::SessionRequest *>(sender()), &QObject::deleteLater);
    handleLoginOrSessionAnswer(request->networkRetCode(), request->sessionErrorCode(), request->sessionStatus(), loginSettings_.authHash(), QString());
}

void LoginController::onServerLocationsAnswer()
{
    QSharedPointer<server_api::ServerListRequest> request(static_cast<server_api::ServerListRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putServerLocationsAnswer(request->networkRetCode(), request->locations(), request->forceDisconnectNodes());
}

void LoginController::onServerCredentialsAnswer()
{
    QSharedPointer<server_api::ServerCredentialsRequest> request(static_cast<server_api::ServerCredentialsRequest *>(sender()), &QObject::deleteLater);
    if (request->protocol().isOpenVpnProtocol())
    {
        getAllConfigsController_->putServerCredentialsOpenVpnAnswer(request->networkRetCode(), request->radiusUsername(), request->radiusPassword());
    }
    else if (request->protocol().isIkev2Protocol())
    {
        getAllConfigsController_->putServerCredentialsIkev2Answer(request->networkRetCode(), request->radiusUsername(), request->radiusPassword());
    }
}

void LoginController::onServerConfigsAnswer()
{
    QSharedPointer<server_api::ServerConfigsRequest> request(static_cast<server_api::ServerConfigsRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putServerConfigsAnswer(request->networkRetCode(), request->ovpnConfig());
}

void LoginController::onPortMapAnswer()
{
    QSharedPointer<server_api::PortMapRequest> request(static_cast<server_api::PortMapRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putPortMapAnswer(request->networkRetCode(), request->portMap());
}

void LoginController::onStaticIpsAnswer()
{
    QSharedPointer<server_api::StaticIpsRequest> request(static_cast<server_api::StaticIpsRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putStaticIpsAnswer(request->networkRetCode(), request->staticIps());
}

void LoginController::tryLoginAgain()
{
    if (!loginSettings_.isAuthHashLogin())
    {
        server_api::BaseRequest *request = serverAPI_->login(loginSettings_.username(), loginSettings_.password(), loginSettings_.code2fa());
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onLoginAnswer);
    }
    else // AUTH_HASH
    {
        server_api::BaseRequest *request = serverAPI_->session(loginSettings_.authHash());
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onSessionAnswer);
    }
}

void LoginController::onAllConfigsReceived(SERVER_API_RET_CODE retCode)
{
    if (retCode == SERVER_RETURN_SUCCESS)
    {
        apiinfo::ApiInfo apiInfo;
        apiInfo.setForceDisconnectNodes(getAllConfigsController_->forceDisconnectNodes_);
        apiInfo.setOvpnConfig(getAllConfigsController_->ovpnConfig_);
        apiInfo.setServerCredentials(getAllConfigsController_->getServerCredentials());
        apiInfo.setLocations(getAllConfigsController_->locations_);
        apiInfo.setPortMap(getAllConfigsController_->portMap_);
        apiInfo.setStaticIps(getAllConfigsController_->staticIps_);
        apiInfo.setSessionStatus(sessionStatus_);
        apiInfo.setAuthHash(newAuthHash_);
        emit finished(LOGIN_RET_SUCCESS, apiInfo, bFromConnectedToVPNState_, QString());
    }
    else if (retCode == SERVER_RETURN_NETWORK_ERROR || retCode == SERVER_RETURN_SSL_ERROR || retCode == SERVER_RETURN_INCORRECT_JSON)
    {
        if (retCode == SERVER_RETURN_SSL_ERROR)
            emit finished(LOGIN_RET_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        else
            emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
    }
    else
    {
        WS_ASSERT(false);
    }
}

void LoginController::handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, server_api::SessionErrorCode sessionErrorCode, const types::SessionStatus &sessionStatus,
                                                 const QString &authHash, const QString &errorMessage)
{
    if (retCode == SERVER_RETURN_SUCCESS)
    {
        if (sessionErrorCode == server_api::SessionErrorCode::kSuccess) {
            sessionStatus_ = sessionStatus;
            newAuthHash_ = authHash;
            getAllConfigs();
        } else if (sessionErrorCode == server_api::SessionErrorCode::kBadUsername) {
            emit finished(LOGIN_RET_BAD_USERNAME, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kMissingCode2FA) {
            emit finished(LOGIN_RET_MISSING_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kBadCode2FA) {
            emit finished(LOGIN_RET_BAD_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kAccountDisabled) {
            emit finished(LOGIN_RET_ACCOUNT_DISABLED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kSessionInvalid) {
            emit finished(LOGIN_RET_SESSION_INVALID, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
        } else {
            WS_ASSERT(false);
            emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
    else if (retCode == SERVER_RETURN_NETWORK_ERROR || retCode == SERVER_RETURN_INCORRECT_JSON || retCode == SERVER_RETURN_SSL_ERROR)
    {
        if (retCode == SERVER_RETURN_SSL_ERROR)
        {
            emit finished(LOGIN_RET_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
        else
        {
            emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
    else
    {
        WS_ASSERT(false);
    }
}

void LoginController::makeLoginRequest()
{
    loginElapsedTimer_.start();
    if (!loginSettings_.isAuthHashLogin())
    {
        server_api::BaseRequest *request = serverAPI_->login(loginSettings_.username(), loginSettings_.password(), loginSettings_.code2fa());
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onLoginAnswer);
    }
    else // AUTH_HASH
    {
        server_api::BaseRequest *request = serverAPI_->session(loginSettings_.authHash());
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onSessionAnswer);
    }
}

void LoginController::getAllConfigs()
{
    SAFE_DELETE(getAllConfigsController_);
    getAllConfigsController_ = new GetAllConfigsController(this);
    connect(getAllConfigsController_, SIGNAL(allConfigsReceived(SERVER_API_RET_CODE)), SLOT(onAllConfigsReceived(SERVER_API_RET_CODE)));

    server_api::BaseRequest *requestConfigs = serverAPI_->serverConfigs(newAuthHash_);
    connect(requestConfigs, &server_api::BaseRequest::finished, this, &LoginController::onServerConfigsAnswer);

    server_api::BaseRequest *requestCredentials = serverAPI_->serverCredentials(newAuthHash_, PROTOCOL::OPENVPN_UDP);
    connect(requestCredentials, &server_api::BaseRequest::finished, this, &LoginController::onServerCredentialsAnswer);

    if (!loginSettings_.getServerCredentials().isInitialized())
    {
        server_api::BaseRequest *requestCredentials = serverAPI_->serverCredentials(newAuthHash_, PROTOCOL::IKEV2);
        connect(requestCredentials, &server_api::BaseRequest::finished, this, &LoginController::onServerCredentialsAnswer);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Using saved server credentials for IKEv2";
        getAllConfigsController_->putServerCredentialsIkev2Answer(SERVER_RETURN_SUCCESS, loginSettings_.getServerCredentials().usernameForIkev2(), loginSettings_.getServerCredentials().passwordForIkev2());
    }

    server_api::BaseRequest *requestLocations = serverAPI_->serverLocations(language_, sessionStatus_.getRevisionHash(), sessionStatus_.isPremium(), sessionStatus_.getAlc());
    connect(requestLocations, &server_api::BaseRequest::finished, this, &LoginController::onServerLocationsAnswer);

    server_api::BaseRequest *requestPortMap = serverAPI_->portMap(newAuthHash_);
    connect(requestPortMap, &server_api::BaseRequest::finished, this, &LoginController::onPortMapAnswer);

    if (sessionStatus_.getStaticIpsCount() > 0)
    {
        server_api::BaseRequest *requestStaticIps = serverAPI_->staticIps(newAuthHash_, GetDeviceId::instance().getDeviceId());
        connect(requestStaticIps, &server_api::BaseRequest::finished, this, &LoginController::onStaticIpsAnswer);
    }
    else
    {
        getAllConfigsController_->putStaticIpsAnswer(SERVER_RETURN_SUCCESS, apiinfo::StaticIps());
    }
}

void LoginController::handleNetworkConnection()
{
    if (networkDetectionManager_->isOnline())
    {
        makeLoginRequest();
    }
    else
    {
        if (waitNetworkConnectivityElapsedTimer_.isValid())
        {
            if (waitNetworkConnectivityElapsedTimer_.elapsed() > MAX_WAIT_CONNECTIVITY_TIMEOUT)
            {
                qCDebug(LOG_BASIC) << "No internet connectivity";
                emit finished(LOGIN_RET_NO_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
            }
            else
            {
                QTimer::singleShot(1000, this, SLOT(handleNetworkConnection()));
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "No internet connectivity, waiting 20 sec";
            waitNetworkConnectivityElapsedTimer_.start();
            QTimer::singleShot(1000, this, SLOT(handleNetworkConnection()));
        }
    }
}
