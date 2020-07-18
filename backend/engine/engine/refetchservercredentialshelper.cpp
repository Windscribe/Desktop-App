#include "refetchservercredentialshelper.h"
#include "Utils/logger.h"

RefetchServerCredentialsHelper::RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, ServerAPI *serverAPI) : QObject(parent),
    authHash_(authHash), serverAPI_(serverAPI), refetchServerCredentialsState_(0)
{
    connect(serverAPI_, SIGNAL(serverCredentialsAnswer(SERVER_API_RET_CODE,QString,QString,ProtocolType,uint)),
                        SLOT(onServerCredentialsAnswer(SERVER_API_RET_CODE,QString,QString,ProtocolType,uint)), Qt::QueuedConnection);
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
        isOpenVpnProtocolReceived_ = false;
        isIkev2ProtocolReceived_ = false;
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP), true);
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_IKEV2), true);
    }
}

void RefetchServerCredentialsHelper::putFail()
{
    if (refetchServerCredentialsState_ < 2)
    {
        isOpenVpnProtocolReceived_ = false;
        isIkev2ProtocolReceived_ = false;
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP), true);
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_IKEV2), true);
    }
    else
    {
        emit finished(false, ServerCredentials());
    }
    refetchServerCredentialsState_++;
}

void RefetchServerCredentialsHelper::onTimerWaitServerAPIReady()
{
    if (serverAPI_->isRequestsEnabled())
    {
        timerWaitServerAPIReady_.stop();
        isOpenVpnProtocolReceived_ = false;
        isIkev2ProtocolReceived_ = false;
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP), true);
        serverAPI_->serverCredentials(authHash_, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_IKEV2), true);
    }
}

void RefetchServerCredentialsHelper::onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword, ProtocolType protocol, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (protocol.isOpenVpnProtocol())
        {
            Q_ASSERT(!isOpenVpnProtocolReceived_);
            isOpenVpnProtocolReceived_ = true;
            retCodeOpenVpn_ = retCode;
            radiusUsernameOpenVpn_ = radiusUsername;
            radiusPasswordOpenVpn_ = radiusPassword;
        }
        else if (protocol.isIkev2Protocol())
        {
            Q_ASSERT(!isIkev2ProtocolReceived_);
            isIkev2ProtocolReceived_ = true;
            retCodeIkev2_ = retCode;
            radiusUsernameIkev2_ = radiusUsername;
            radiusPasswordIkev2_ = radiusPassword;
        }
        else
        {
            Q_ASSERT(false);
        }

        // both credentials answer received
        if (isOpenVpnProtocolReceived_ && isIkev2ProtocolReceived_)
        {
            if (retCodeOpenVpn_ != SERVER_RETURN_API_NOT_READY && retCodeIkev2_ != SERVER_RETURN_API_NOT_READY)
            {
                qCDebug(LOG_BASIC) << "onServerCredentialsAnswer, retCode =" << retCodeOpenVpn_ << "," << retCodeIkev2_  << ", refetchServerCredentialsState_ =" << refetchServerCredentialsState_;
                if (!radiusUsernameOpenVpn_.isEmpty() && !radiusPasswordOpenVpn_.isEmpty() &&
                    !radiusUsernameIkev2_.isEmpty() && !radiusPasswordIkev2_.isEmpty())
                {
                    emit finished(true, ServerCredentials(radiusUsernameOpenVpn_, radiusPasswordOpenVpn_, radiusUsernameIkev2_, radiusPasswordIkev2_));
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
}
