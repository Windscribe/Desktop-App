#include "refetchservercredentialshelper.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

RefetchServerCredentialsHelper::RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, ServerAPI *serverAPI) : QObject(parent),
    authHash_(authHash), serverAPI_(serverAPI), refetchServerCredentialsState_(0),
    isOpenVpnProtocolReceived_(false), isIkev2ProtocolReceived_(false), isServerConfigsAnswerReceived_(false)
{
    connect(serverAPI_, &ServerAPI::serverCredentialsAnswer, this, &RefetchServerCredentialsHelper::onServerCredentialsAnswer, Qt::QueuedConnection);
    connect(serverAPI_, &ServerAPI::serverConfigsAnswer, this, &RefetchServerCredentialsHelper::onServerConfigsAnswer, Qt::QueuedConnection);
    serverApiUserRole_ = serverAPI_->getAvailableUserRole();
    connect(&timerWaitServerAPIReady_, SIGNAL(timeout()), SLOT(onTimerWaitServerAPIReady()));
}

RefetchServerCredentialsHelper::~RefetchServerCredentialsHelper()
{
}

void RefetchServerCredentialsHelper::startRefetch()
{
    refetchServerCredentialsState_ = 0;
    if (!serverAPI_->isRequestsEnabled())
    {
        timerWaitServerAPIReady_.start(1000);
    }
    else
    {
        fetchServerCredentials();
    }
}

void RefetchServerCredentialsHelper::putFail()
{
    if (refetchServerCredentialsState_ < 2)
    {
        fetchServerCredentials();
    }
    else
    {
        emit finished(false, apiinfo::ServerCredentials(), QString());
    }
    refetchServerCredentialsState_++;
}

void RefetchServerCredentialsHelper::onTimerWaitServerAPIReady()
{
    if (serverAPI_->isRequestsEnabled())
    {
        timerWaitServerAPIReady_.stop();
        fetchServerCredentials();
    }
}

void RefetchServerCredentialsHelper::onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword,
                                                               PROTOCOL protocol, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (protocol.isOpenVpnProtocol())
        {
            WS_ASSERT(!isOpenVpnProtocolReceived_);
            isOpenVpnProtocolReceived_ = true;
            retCodeOpenVpn_ = retCode;
            radiusUsernameOpenVpn_ = radiusUsername;
            radiusPasswordOpenVpn_ = radiusPassword;
        }
        else if (protocol.isIkev2Protocol())
        {
            WS_ASSERT(!isIkev2ProtocolReceived_);
            isIkev2ProtocolReceived_ = true;
            retCodeIkev2_ = retCode;
            radiusUsernameIkev2_ = radiusUsername;
            radiusPasswordIkev2_ = radiusPassword;
        }
        else
        {
            WS_ASSERT(false);
        }

        checkFinished();
    }
}

void RefetchServerCredentialsHelper::onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        isServerConfigsAnswerReceived_ = true;
        retCodeServerConfigs_ = retCode;
        serverConfig_ = config;
        checkFinished();
    }
}

void RefetchServerCredentialsHelper::fetchServerCredentials()
{
    isOpenVpnProtocolReceived_ = false;
    isIkev2ProtocolReceived_ = false;
    isServerConfigsAnswerReceived_ = false;
    retCodeOpenVpn_ = SERVER_RETURN_API_NOT_READY;
    retCodeIkev2_ = SERVER_RETURN_API_NOT_READY;
    retCodeServerConfigs_ = SERVER_RETURN_API_NOT_READY;
    radiusUsernameOpenVpn_.clear();
    radiusPasswordOpenVpn_.clear();
    radiusUsernameIkev2_.clear();
    radiusUsernameIkev2_.clear();
    serverConfig_.clear();
    serverAPI_->serverConfigs(authHash_, serverApiUserRole_, true);
    serverAPI_->serverCredentials(authHash_, serverApiUserRole_, PROTOCOL::OPENVPN_UDP, true);
    serverAPI_->serverCredentials(authHash_, serverApiUserRole_, PROTOCOL::IKEV2, true);
}

void RefetchServerCredentialsHelper::checkFinished()
{
    if (isOpenVpnProtocolReceived_ && isIkev2ProtocolReceived_ && isServerConfigsAnswerReceived_)
    {
        if (retCodeOpenVpn_ != SERVER_RETURN_API_NOT_READY && retCodeIkev2_ != SERVER_RETURN_API_NOT_READY && retCodeServerConfigs_ != SERVER_RETURN_API_NOT_READY)
        {
            qCDebug(LOG_BASIC) << "Refetch server config and credentials, retCode =" << retCodeOpenVpn_ << "," << retCodeIkev2_ << "," << retCodeServerConfigs_ << ", refetchServerCredentialsState_ =" << refetchServerCredentialsState_;

            if (!radiusUsernameOpenVpn_.isEmpty() && !radiusPasswordOpenVpn_.isEmpty() &&
                !radiusUsernameIkev2_.isEmpty() && !radiusPasswordIkev2_.isEmpty() && !serverConfig_.isEmpty())
            {
                emit finished(true, apiinfo::ServerCredentials(radiusUsernameOpenVpn_, radiusPasswordOpenVpn_, radiusUsernameIkev2_, radiusPasswordIkev2_), serverConfig_);
            }
            else
            {
                putFail();
            }
        }
        else
        {
            if (!serverAPI_->isRequestsEnabled())
            {
                timerWaitServerAPIReady_.start(1000);
            }
            else
            {
                putFail();
            }
        }
    }
}
