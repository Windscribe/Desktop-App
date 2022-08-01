#include "utils/logger.h"
#include <QTimer>
#include "logincontroller.h"
#include "engine/helper/ihelper.h"
#include "engine/serverapi/serverapi.h"
#include "utils/utils.h"
#include "utils/hardcodedsettings.h"
#include "engine/getdeviceid.h"
#include "version/appversion.h"

LoginController::LoginController(QObject *parent,  IHelper *helper,
                                 INetworkDetectionManager *networkDetectionManager, ServerAPI *serverAPI,
                                 const QString &language, types::ProtocolType protocol) : QObject(parent),
    helper_(helper), serverAPI_(serverAPI),
    getApiAccessIps_(NULL), networkDetectionManager_(networkDetectionManager), language_(language),
    protocol_(protocol), bFromConnectedToVPNState_(false), getAllConfigsController_(NULL),
    loginStep_(LOGIN_STEP1), readyForNetworkRequestsEmitted_(false)
{
    connect(serverAPI_, &ServerAPI::loginAnswer, this, &LoginController::onLoginAnswer, Qt::QueuedConnection);
    connect(serverAPI_, &ServerAPI::sessionAnswer, this, &LoginController::onSessionAnswer, Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(serverConfigsAnswer(SERVER_API_RET_CODE,QString, uint)), SLOT(onServerConfigsAnswer(SERVER_API_RET_CODE,QString, uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(serverCredentialsAnswer(SERVER_API_RET_CODE,QString,QString, types::ProtocolType, uint)), SLOT(onServerCredentialsAnswer(SERVER_API_RET_CODE,QString,QString, types::ProtocolType, uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(portMapAnswer(SERVER_API_RET_CODE, types::PortMap, uint)), SLOT(onPortMapAnswer(SERVER_API_RET_CODE, types::PortMap, uint)), Qt::QueuedConnection);

    connect(serverAPI_, SIGNAL(serverLocationsAnswer(SERVER_API_RET_CODE,QVector<types::Location>,QStringList, uint)),
                            SLOT(onServerLocationsAnswer(SERVER_API_RET_CODE, QVector<types::Location>,QStringList, uint)), Qt::QueuedConnection);

    connect(serverAPI_, SIGNAL(staticIpsAnswer(SERVER_API_RET_CODE, types::StaticIps, uint)), SLOT(onStaticIpsAnswer(SERVER_API_RET_CODE, types::StaticIps, uint)), Qt::QueuedConnection);

    serverApiUserRole_ = serverAPI_->getAvailableUserRole();
}

LoginController::~LoginController()
{
}

void LoginController::startLoginProcess(const types::LoginSettings &loginSettings, const types::DnsResolutionSettings &dnsResolutionSettings, bool bFromConnectedToVPNState)
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

void LoginController::onLoginAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, const QString &authHash, uint userRole, const QString &errorMessage)
{
    if (userRole == serverApiUserRole_)
    {
        handleLoginOrSessionAnswer(retCode, sessionStatus, authHash, errorMessage);
    }
}

void LoginController::onSessionAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        handleLoginOrSessionAnswer(retCode, sessionStatus, loginSettings_.authHash(), QString());
    }
}

void LoginController::onServerLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<types::Location> &serverLocations, QStringList forceDisconnectNodes, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        getAllConfigsController_->putServerLocationsAnswer(retCode, serverLocations, forceDisconnectNodes);
    }
}

void LoginController::onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword, types::ProtocolType protocol, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (protocol.isOpenVpnProtocol())
        {
            getAllConfigsController_->putServerCredentialsOpenVpnAnswer(retCode, radiusUsername, radiusPassword);
        }
        else if (protocol.isIkev2Protocol())
        {
            getAllConfigsController_->putServerCredentialsIkev2Answer(retCode, radiusUsername, radiusPassword);
        }
    }
}

void LoginController::onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        getAllConfigsController_->putServerConfigsAnswer(retCode, config);
    }
}

void LoginController::onPortMapAnswer(SERVER_API_RET_CODE retCode, const types::PortMap &portMap, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        getAllConfigsController_->putPortMapAnswer(retCode, portMap);
    }
}

void LoginController::onStaticIpsAnswer(SERVER_API_RET_CODE retCode, const types::StaticIps &staticIps, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        getAllConfigsController_->putStaticIpsAnswer(retCode, staticIps);
    }
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
            emit finished(LOGIN_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
    else if (retCode == SERVER_RETURN_PROXY_AUTH_FAILED)
    {
        emit finished(LOGIN_PROXY_AUTH_NEED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
    }
    else // failed
    {
        if (retCode == SERVER_RETURN_SSL_ERROR)
        {
            retCodesForLoginSteps_ << SERVER_RETURN_SSL_ERROR;
        }

        if (isAllSslErrors())
        {
            emit finished(LOGIN_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
        else
        {
            emit finished(LOGIN_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
}

void LoginController::tryLoginAgain()
{
    if (!loginSettings_.isAuthHashLogin())
    {
        serverAPI_->login(loginSettings_.username(), loginSettings_.password(), loginSettings_.code2fa(), serverApiUserRole_, false);
    }
    else // AUTH_HASH
    {
        serverAPI_->session(loginSettings_.authHash(), serverApiUserRole_, false);
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
        emit finished(LOGIN_SUCCESS, apiInfo, bFromConnectedToVPNState_, QString());
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
        Q_ASSERT(false);
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
        emit finished(LOGIN_BAD_USERNAME, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_MISSING_CODE2FA)
    {
        emit finished(LOGIN_MISSING_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_BAD_CODE2FA)
    {
        emit finished(LOGIN_BAD_CODE2FA, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_PROXY_AUTH_FAILED)
    {
        emit finished(LOGIN_PROXY_AUTH_NEED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_ACCOUNT_DISABLED)
    {
        emit finished(LOGIN_ACCOUNT_DISABLED, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else if (retCode == SERVER_RETURN_SESSION_INVALID)
    {
        emit finished(LOGIN_SESSION_INVALID, apiinfo::ApiInfo(), bFromConnectedToVPNState_, errorMessage);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void LoginController::makeLoginRequest(const QString &hostname)
{
    qCDebug(LOG_BASIC) << "Try login with hostname:" << hostname;
    serverAPI_->setHostname(hostname);
    loginElapsedTimer_.start();
    if (!loginSettings_.isAuthHashLogin())
    {
        serverAPI_->login(loginSettings_.username(), loginSettings_.password(), loginSettings_.code2fa(), serverApiUserRole_, false);
    }
    else // AUTH_HASH
    {
        serverAPI_->session(loginSettings_.authHash(), serverApiUserRole_, false);
    }
}

void LoginController::makeApiAccessRequest()
{
    Q_ASSERT(getApiAccessIps_ == NULL);
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
            makeLoginRequest(HardcodedSettings::instance().generateDomain("api."));
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
                    emit finished(LOGIN_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
                }
                else
                {
                    emit finished(LOGIN_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
                }
            }
        }
    }
    else
    {
        if (retCode == SERVER_RETURN_SSL_ERROR)
        {
            emit finished(LOGIN_SSL_ERROR, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
        else
        {
            emit finished(LOGIN_NO_API_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
        }
    }
}

void LoginController::getAllConfigs()
{
    SAFE_DELETE(getAllConfigsController_);
    getAllConfigsController_ = new GetAllConfigsController(this);
    connect(getAllConfigsController_, SIGNAL(allConfigsReceived(SERVER_API_RET_CODE)), SLOT(onAllConfigsReceived(SERVER_API_RET_CODE)));
    serverAPI_->serverConfigs(newAuthHash_, serverApiUserRole_, false);
    serverAPI_->serverCredentials(newAuthHash_, serverApiUserRole_, types::ProtocolType::PROTOCOL_OPENVPN_UDP, false);

    if (!loginSettings_.getServerCredentials().isInitialized())
    {
        serverAPI_->serverCredentials(newAuthHash_, serverApiUserRole_, types::ProtocolType::PROTOCOL_IKEV2, false);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Using saved server credentials for IKEv2";
        getAllConfigsController_->putServerCredentialsIkev2Answer(SERVER_RETURN_SUCCESS, loginSettings_.getServerCredentials().usernameForIkev2(), loginSettings_.getServerCredentials().passwordForIkev2());
    }

    serverAPI_->serverLocations(newAuthHash_, language_, serverApiUserRole_, false, sessionStatus_.getRevisionHash(), sessionStatus_.isPro(), protocol_, sessionStatus_.getAlc());
    serverAPI_->portMap(newAuthHash_, serverApiUserRole_, false);
    if (sessionStatus_.getStaticIpsCount() > 0)
    {
        serverAPI_->staticIps(newAuthHash_, GetDeviceId::instance().getDeviceId(), serverApiUserRole_, false);
    }
    else
    {
        getAllConfigsController_->putStaticIpsAnswer(SERVER_RETURN_SUCCESS, types::StaticIps());
    }
}

void LoginController::handleNetworkConnection()
{
    if (networkDetectionManager_->isOnline())
    {
        if (dnsResolutionSettings_.getIsAutomatic())
        {
            makeLoginRequest(HardcodedSettings::instance().serverApiUrl());
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
                emit finished(LOGIN_NO_CONNECTIVITY, apiinfo::ApiInfo(), bFromConnectedToVPNState_, QString());
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
