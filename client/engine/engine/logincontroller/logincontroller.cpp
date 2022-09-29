#include "utils/logger.h"
#include <QTimer>
#include "logincontroller.h"
#include "engine/helper/ihelper.h"
#include "engine/serverapi/serverapi.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "utils/hardcodedsettings.h"
#include "engine/getdeviceid.h"
#include "version/appversion.h"
#include "engine/serverapi/requests/loginrequest.h"
#include "engine/serverapi/requests/sessionrequest.h"
#include "engine/serverapi/requests/serverlistrequest.h"
#include "engine/serverapi/requests/servercredentialsrequest.h"
#include "engine/serverapi/requests/serverconfigsrequest.h"
#include "engine/serverapi/requests/portmaprequest.h"
#include "engine/serverapi/requests/staticipsrequest.h"

LoginController::LoginController(QObject *parent,  IHelper *helper,
                                 INetworkDetectionManager *networkDetectionManager, server_api::ServerAPI *serverAPI,
                                 const QString &language, PROTOCOL protocol) : QObject(parent),
    helper_(helper), serverAPI_(serverAPI),
    getApiAccessIps_(NULL), networkDetectionManager_(networkDetectionManager), language_(language),
    protocol_(protocol), bFromConnectedToVPNState_(false), getAllConfigsController_(NULL),
    loginStep_(LOGIN_STEP1), readyForNetworkRequestsEmitted_(false)
{
}

LoginController::~LoginController()
{
}

void LoginController::startLoginProcess(const LoginSettings &loginSettings, const types::DnsResolutionSettings &dnsResolutionSettings, bool bFromConnectedToVPNState)
{
    if (loginSettings.isAuthHashLogin())
    {
         qCDebug(LOG_BASIC) << "Start login process with authHash, bFromConnectedToVPNState =" << bFromConnectedToVPNState;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Start login process with username and password, bFromConnectedToVPNState =" << bFromConnectedToVPNState;
    }

    retCodesForLoginSteps_.clear();
    loginSettings_ = loginSettings;

    dnsResolutionSettings_ = dnsResolutionSettings;
    bFromConnectedToVPNState_ = bFromConnectedToVPNState;

    loginStep_ = LOGIN_STEP1;
    readyForNetworkRequestsEmitted_ = false;

    handleNetworkConnection();
}

void LoginController::onLoginAnswer()
{
    QSharedPointer<server_api::LoginRequest> request(static_cast<server_api::LoginRequest *>(sender()), &QObject::deleteLater);
    handleLoginOrSessionAnswer(request->retCode(), request->sessionStatus(), request->authHash(), request->errorMessage());
}

void LoginController::onSessionAnswer()
{
    QSharedPointer<server_api::SessionRequest> request(static_cast<server_api::SessionRequest *>(sender()), &QObject::deleteLater);
    handleLoginOrSessionAnswer(request->retCode(), request->sessionStatus(), loginSettings_.authHash(), QString());
}

void LoginController::onServerLocationsAnswer()
{
    QSharedPointer<server_api::ServerListRequest> request(static_cast<server_api::ServerListRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putServerLocationsAnswer(request->retCode(), request->locations(), request->forceDisconnectNodes());
}

void LoginController::onServerCredentialsAnswer()
{
    QSharedPointer<server_api::ServerCredentialsRequest> request(static_cast<server_api::ServerCredentialsRequest *>(sender()), &QObject::deleteLater);
    if (request->protocol().isOpenVpnProtocol())
    {
        getAllConfigsController_->putServerCredentialsOpenVpnAnswer(request->retCode(), request->radiusUsername(), request->radiusPassword());
    }
    else if (request->protocol().isIkev2Protocol())
    {
        getAllConfigsController_->putServerCredentialsIkev2Answer(request->retCode(), request->radiusUsername(), request->radiusPassword());
    }
}

void LoginController::onServerConfigsAnswer()
{
    QSharedPointer<server_api::ServerConfigsRequest> request(static_cast<server_api::ServerConfigsRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putServerConfigsAnswer(request->retCode(), request->ovpnConfig());
}

void LoginController::onPortMapAnswer()
{
    QSharedPointer<server_api::PortMapRequest> request(static_cast<server_api::PortMapRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putPortMapAnswer(request->retCode(), request->portMap());
}

void LoginController::onStaticIpsAnswer()
{
    QSharedPointer<server_api::StaticIpsRequest> request(static_cast<server_api::StaticIpsRequest *>(sender()), &QObject::deleteLater);
    getAllConfigsController_->putStaticIpsAnswer(request->retCode(), request->staticIps());
}

void LoginController::onGetApiAccessIpsFinished(SERVER_API_RET_CODE retCode, const QStringList &hosts)
{
    qCDebug(LOG_BASIC) << "LoginController::onGetApiAccessIpsFinished, retCode=" << retCode << ", hosts=" << hosts;
    if (retCode == SERVER_RETURN_SUCCESS)
    {
        if (hosts.count() > 0)
        {
            ipsForStep3_ = hosts;
            //emit stepMessage(tr("Trying Backup Endpoints 2/2"));
            emit stepMessage(LOGIN_MESSAGE_TRYING_BACKUP2);
            makeLoginRequest(selectRandomIpForStep3());
        }
        else
        {
            emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
    else if (retCode == SERVER_RETURN_PROXY_AUTH_FAILED)
    {
        emit finished(LOGIN_RET_PROXY_AUTH_NEED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
    }
    else // failed
    {
        if (retCode == SERVER_RETURN_SSL_ERROR)
        {
            retCodesForLoginSteps_ << SERVER_RETURN_SSL_ERROR;
        }

        if (isAllSslErrors())
        {
            emit finished(LOGIN_RET_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
        else
        {
            emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
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
        server_api::BaseRequest *request = serverAPI_->session(loginSettings_.authHash(), false);
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
        // try again
        /*if (loginElapsedTimer_.elapsed() > MAX_WAIT_LOGIN_TIMEOUT)
        {*/
             handleNextLoginAfterFail(retCode);
        /*}
        else
        {
            QTimer::singleShot(1000, this, SLOT(getAllConfigs()));
        }*/
    }
    else
    {
        WS_ASSERT(false);
    }
}

void LoginController::handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus,
                                                 const QString &authHash, const QString &errorMessage)
{
    if (retCode == SERVER_RETURN_SUCCESS)
    {
        if (!readyForNetworkRequestsEmitted_)
        {
            readyForNetworkRequestsEmitted_ = true;
            emit readyForNetworkRequests();
        }
        sessionStatus_ = sessionStatus;
        newAuthHash_ = authHash;
        getAllConfigs();
    }
    else if (retCode == SERVER_RETURN_NETWORK_ERROR || retCode == SERVER_RETURN_INCORRECT_JSON || retCode == SERVER_RETURN_SSL_ERROR)
    {
        if (loginElapsedTimer_.elapsed() > MAX_WAIT_LOGIN_TIMEOUT)
        {
            handleNextLoginAfterFail(retCode);
        }
        else
        {
            QTimer::singleShot(1000, this, SLOT(tryLoginAgain()));
        }
    }
    else if (retCode == SERVER_RETURN_BAD_USERNAME)
    {
        emit finished(LOGIN_RET_BAD_USERNAME, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_MISSING_CODE2FA)
    {
        emit finished(LOGIN_RET_MISSING_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_BAD_CODE2FA)
    {
        emit finished(LOGIN_RET_BAD_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_PROXY_AUTH_FAILED)
    {
        emit finished(LOGIN_RET_PROXY_AUTH_NEED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_ACCOUNT_DISABLED)
    {
        emit finished(LOGIN_RET_ACCOUNT_DISABLED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_SESSION_INVALID)
    {
        emit finished(LOGIN_RET_SESSION_INVALID, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else
    {
        WS_ASSERT(false);
    }
}

void LoginController::makeLoginRequest(const QString &hostname)
{
    qCDebug(LOG_BASIC) << "Try login with hostname:" << hostname;
    serverAPI_->setHostname(hostname);
    loginElapsedTimer_.start();
    if (!loginSettings_.isAuthHashLogin())
    {
        server_api::BaseRequest *request = serverAPI_->login(loginSettings_.username(), loginSettings_.password(), loginSettings_.code2fa());
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onLoginAnswer);
    }
    else // AUTH_HASH
    {
        server_api::BaseRequest *request = serverAPI_->session(loginSettings_.authHash(), false);
        connect(request, &server_api::BaseRequest::finished, this, &LoginController::onSessionAnswer);
    }
}

void LoginController::makeApiAccessRequest()
{
    WS_ASSERT(getApiAccessIps_ == NULL);
    getApiAccessIps_ = new GetApiAccessIps(this, serverAPI_);
    connect(getApiAccessIps_, SIGNAL(finished(SERVER_API_RET_CODE,QStringList)), SLOT(onGetApiAccessIpsFinished(SERVER_API_RET_CODE,QStringList)));
    getApiAccessIps_->get();
}

QString LoginController::selectRandomIpForStep3()
{
    if (ipsForStep3_.count() > 0)
    {
        int randomInd = Utils::generateIntegerRandom(0, ipsForStep3_.count() - 1); // random number from 0 to ipsForStep3_.count() - 1
        QString randIp = ipsForStep3_[randomInd];
        ipsForStep3_.removeAt(randomInd);
        return randIp;
    }
    else
    {
        return "";
    }
}

bool LoginController::isAllSslErrors() const
{
    bool bAllSslErrors = true;
    for (SERVER_API_RET_CODE rc : retCodesForLoginSteps_)
    {
        if (rc != SERVER_RETURN_SSL_ERROR)
        {
             bAllSslErrors = false;
             break;
        }
    }
    return bAllSslErrors;
}

void LoginController::handleNextLoginAfterFail(SERVER_API_RET_CODE retCode)
{
    // We break the staging functionality to not try the hashed domains if the initial login fails, as the hashed domains will hit the production environment.
    if (!AppVersion::instance().isStaging() && dnsResolutionSettings_.getIsAutomatic())
    {
        if (loginStep_ == LOGIN_STEP1)
        {
            retCodesForLoginSteps_ << retCode;
            loginStep_ = LOGIN_STEP2;
            //emit stepMessage(tr("Trying Backup Endpoints 1/2"));
            emit stepMessage(LOGIN_MESSAGE_TRYING_BACKUP1);
            makeLoginRequest(HardcodedSettings::instance().generateDomain());
        }
        else if (loginStep_ == LOGIN_STEP2)
        {
            retCodesForLoginSteps_ << retCode;
            loginStep_ = LOGIN_STEP3;
            makeApiAccessRequest();
        }
        else if (loginStep_ == LOGIN_STEP3)
        {
            retCodesForLoginSteps_ << retCode;
            QString nextHostIp = selectRandomIpForStep3();
            if (!nextHostIp.isEmpty())
            {
                makeLoginRequest(nextHostIp);
            }
            else
            {
                if (isAllSslErrors())
                {
                    emit finished(LOGIN_RET_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
                }
                else
                {
                    emit finished(LOGIN_RET_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
                }
            }
        }
    }
    else
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
}

void LoginController::getAllConfigs()
{
    SAFE_DELETE(getAllConfigsController_);
    getAllConfigsController_ = new GetAllConfigsController(this);
    connect(getAllConfigsController_, SIGNAL(allConfigsReceived(SERVER_API_RET_CODE)), SLOT(onAllConfigsReceived(SERVER_API_RET_CODE)));

    server_api::BaseRequest *requestConfigs = serverAPI_->serverConfigs(newAuthHash_, false);
    connect(requestConfigs, &server_api::BaseRequest::finished, this, &LoginController::onServerConfigsAnswer);

    server_api::BaseRequest *requestCredentials = serverAPI_->serverCredentials(newAuthHash_, PROTOCOL::OPENVPN_UDP, false);
    connect(requestCredentials, &server_api::BaseRequest::finished, this, &LoginController::onServerCredentialsAnswer);

    if (!loginSettings_.getServerCredentials().isInitialized())
    {
        server_api::BaseRequest *requestCredentials = serverAPI_->serverCredentials(newAuthHash_, PROTOCOL::IKEV2, false);
        connect(requestCredentials, &server_api::BaseRequest::finished, this, &LoginController::onServerCredentialsAnswer);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Using saved server credentials for IKEv2";
        getAllConfigsController_->putServerCredentialsIkev2Answer(SERVER_RETURN_SUCCESS, loginSettings_.getServerCredentials().usernameForIkev2(), loginSettings_.getServerCredentials().passwordForIkev2());
    }

    server_api::BaseRequest *requestLocations = serverAPI_->serverLocations(language_, false, sessionStatus_.getRevisionHash(), sessionStatus_.isPremium(), protocol_, sessionStatus_.getAlc());
    connect(requestLocations, &server_api::BaseRequest::finished, this, &LoginController::onServerLocationsAnswer);

    server_api::BaseRequest *requestPortMap = serverAPI_->portMap(newAuthHash_, false);
    connect(requestPortMap, &server_api::BaseRequest::finished, this, &LoginController::onPortMapAnswer);

    if (sessionStatus_.getStaticIpsCount() > 0)
    {
        server_api::BaseRequest *requestStaticIps = serverAPI_->staticIps(newAuthHash_, GetDeviceId::instance().getDeviceId(), false);
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
        if (dnsResolutionSettings_.getIsAutomatic())
        {
            makeLoginRequest(HardcodedSettings::instance().serverDomains().at(0));
        }
        else
        {
            makeLoginRequest(dnsResolutionSettings_.getManualIp());
        }
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
