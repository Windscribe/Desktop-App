#include "refetchservercredentialshelper.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "engine/serverapi/requests/servercredentialsrequest.h"
#include "engine/serverapi/requests/serverconfigsrequest.h"

RefetchServerCredentialsHelper::RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, server_api::ServerAPI *serverAPI) : QObject(parent),
    authHash_(authHash), serverAPI_(serverAPI), refetchServerCredentialsState_(0),
    isOpenVpnProtocolReceived_(false), isIkev2ProtocolReceived_(false), isServerConfigsAnswerReceived_(false)
{
}

RefetchServerCredentialsHelper::~RefetchServerCredentialsHelper()
{
}

void RefetchServerCredentialsHelper::startRefetch()
{
    refetchServerCredentialsState_ = 0;
    fetchServerCredentials();
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

void RefetchServerCredentialsHelper::onServerCredentialsAnswer()
{
    QSharedPointer<server_api::ServerCredentialsRequest> request(static_cast<server_api::ServerCredentialsRequest *>(sender()), &QObject::deleteLater);

    if (request->protocol().isOpenVpnProtocol())
    {
        WS_ASSERT(!isOpenVpnProtocolReceived_);
        isOpenVpnProtocolReceived_ = true;
        retCodeOpenVpn_ = request->networkRetCode();
        radiusUsernameOpenVpn_ = request->radiusUsername();
        radiusPasswordOpenVpn_ = request->radiusPassword();
    }
    else if (request->protocol().isIkev2Protocol())
    {
        WS_ASSERT(!isIkev2ProtocolReceived_);
        isIkev2ProtocolReceived_ = true;
        retCodeIkev2_ = request->networkRetCode();
        radiusUsernameIkev2_ = request->radiusUsername();
        radiusPasswordIkev2_ = request->radiusPassword();
    }
    else
    {
        WS_ASSERT(false);
    }

    checkFinished();
}

void RefetchServerCredentialsHelper::onServerConfigsAnswer()
{
    QSharedPointer<server_api::ServerConfigsRequest> request(static_cast<server_api::ServerConfigsRequest *>(sender()), &QObject::deleteLater);
    isServerConfigsAnswerReceived_ = true;
    retCodeServerConfigs_ = request->networkRetCode();
    serverConfig_ = request->ovpnConfig();
    checkFinished();
}

void RefetchServerCredentialsHelper::fetchServerCredentials()
{
    isOpenVpnProtocolReceived_ = false;
    isIkev2ProtocolReceived_ = false;
    isServerConfigsAnswerReceived_ = false;
    retCodeOpenVpn_ = SERVER_RETURN_FAILOVER_FAILED;
    retCodeIkev2_ = SERVER_RETURN_FAILOVER_FAILED;
    retCodeServerConfigs_ = SERVER_RETURN_FAILOVER_FAILED;
    radiusUsernameOpenVpn_.clear();
    radiusPasswordOpenVpn_.clear();
    radiusUsernameIkev2_.clear();
    radiusUsernameIkev2_.clear();
    serverConfig_.clear();
    {
        server_api::BaseRequest *request = serverAPI_->serverConfigs(authHash_);
        connect(request, &server_api::BaseRequest::finished, this, &RefetchServerCredentialsHelper::onServerConfigsAnswer);
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverCredentials(authHash_, PROTOCOL::OPENVPN_UDP);
        connect(request, &server_api::BaseRequest::finished, this, &RefetchServerCredentialsHelper::onServerCredentialsAnswer);
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverCredentials(authHash_, PROTOCOL::IKEV2);
        connect(request, &server_api::BaseRequest::finished, this, &RefetchServerCredentialsHelper::onServerCredentialsAnswer);
    }

}

void RefetchServerCredentialsHelper::checkFinished()
{
    if (isOpenVpnProtocolReceived_ && isIkev2ProtocolReceived_ && isServerConfigsAnswerReceived_)
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
}
