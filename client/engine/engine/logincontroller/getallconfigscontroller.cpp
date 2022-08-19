#include "getallconfigscontroller.h"
#include <QThread>

GetAllConfigsController::GetAllConfigsController(QObject *parent) : QObject(parent), bServerLocationsAnswerReceived_(false), bServerCredentialsAnswerOpenVpnReceived_(false),
    bServerCredentialsAnswerIkev2Received_(false),
    bServerConfigsAnswerReceived_(false), bPortMapAnswerReceived_(false), bStaticIpsAnswerReceived_(false)
{

}

void GetAllConfigsController::putServerLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Location> &locations, const QStringList &forceDisconnectNodes)
{
    serverLocationsRetCode_ = retCode;
    locations_ = locations;
    forceDisconnectNodes_ = forceDisconnectNodes;
    bServerLocationsAnswerReceived_ = true;
    handleComplete();
}

void GetAllConfigsController::putServerCredentialsOpenVpnAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword)
{
    serverCredentialsOpenVpnRetCode_ = retCode;
    radiusUsernameOpenVpn_ = radiusUsername;
    radiusPasswordOpenVpn_ = radiusPassword;
    bServerCredentialsAnswerOpenVpnReceived_ = true;
    handleComplete();
}

void GetAllConfigsController::putServerCredentialsIkev2Answer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword)
{
    serverCredentialsIkev2RetCode_ = retCode;
    radiusUsernameIkev2_ = radiusUsername;
    radiusPasswordIkev2_ = radiusPassword;
    bServerCredentialsAnswerIkev2Received_ = true;
    handleComplete();
}

void GetAllConfigsController::putServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config)
{
    serverConfigsRetCode_ = retCode;
    ovpnConfig_ = config;
    bServerConfigsAnswerReceived_ = true;
    handleComplete();
}

void GetAllConfigsController::putPortMapAnswer(SERVER_API_RET_CODE retCode, const types::PortMap &portMap)
{
    portMapRetCode_ = retCode;
    portMap_ = portMap;
    bPortMapAnswerReceived_ = true;
    handleComplete();
}

void GetAllConfigsController::putStaticIpsAnswer(SERVER_API_RET_CODE retCode, const apiinfo::StaticIps &staticIps)
{
    staticIpsRetCode_ = retCode;
    staticIps_ = staticIps;
    bStaticIpsAnswerReceived_ = true;
    handleComplete();
}

apiinfo::ServerCredentials GetAllConfigsController::getServerCredentials() const
{
    return apiinfo::ServerCredentials(radiusUsernameOpenVpn_, radiusPasswordOpenVpn_, radiusUsernameIkev2_, radiusPasswordIkev2_);
}

void GetAllConfigsController::handleComplete()
{
    if (bServerLocationsAnswerReceived_ && bServerCredentialsAnswerOpenVpnReceived_ && bServerCredentialsAnswerIkev2Received_ &&
        bServerConfigsAnswerReceived_ && bPortMapAnswerReceived_ && bStaticIpsAnswerReceived_)
    {
        if (serverLocationsRetCode_ == SERVER_RETURN_SSL_ERROR || serverCredentialsOpenVpnRetCode_ == SERVER_RETURN_SSL_ERROR ||
                serverCredentialsIkev2RetCode_ == SERVER_RETURN_SSL_ERROR ||
                serverConfigsRetCode_ == SERVER_RETURN_SSL_ERROR ||
                portMapRetCode_ == SERVER_RETURN_SSL_ERROR ||
                staticIpsRetCode_ == SERVER_RETURN_SSL_ERROR)
        {
            emit allConfigsReceived(SERVER_RETURN_SSL_ERROR);
        }
        else if (serverLocationsRetCode_ == SERVER_RETURN_NETWORK_ERROR || serverCredentialsOpenVpnRetCode_ == SERVER_RETURN_NETWORK_ERROR ||
                 serverCredentialsIkev2RetCode_ == SERVER_RETURN_NETWORK_ERROR ||
                 serverConfigsRetCode_ == SERVER_RETURN_NETWORK_ERROR ||
                 portMapRetCode_ == SERVER_RETURN_NETWORK_ERROR ||
                 staticIpsRetCode_ == SERVER_RETURN_NETWORK_ERROR)
        {
            emit allConfigsReceived(SERVER_RETURN_NETWORK_ERROR);
        }
        else if (serverLocationsRetCode_ == SERVER_RETURN_INCORRECT_JSON || serverCredentialsOpenVpnRetCode_ == SERVER_RETURN_INCORRECT_JSON ||
                 serverCredentialsIkev2RetCode_ == SERVER_RETURN_INCORRECT_JSON ||
                 serverConfigsRetCode_ == SERVER_RETURN_INCORRECT_JSON ||
                 portMapRetCode_ == SERVER_RETURN_INCORRECT_JSON ||
                 staticIpsRetCode_ == SERVER_RETURN_INCORRECT_JSON)
        {
            emit allConfigsReceived(SERVER_RETURN_INCORRECT_JSON);
        }
        else if (serverLocationsRetCode_ == SERVER_RETURN_SUCCESS && serverCredentialsOpenVpnRetCode_ == SERVER_RETURN_SUCCESS &&
                 serverCredentialsIkev2RetCode_ == SERVER_RETURN_SUCCESS &&
                 serverConfigsRetCode_ == SERVER_RETURN_SUCCESS &&
                 portMapRetCode_ == SERVER_RETURN_SUCCESS &&
                 staticIpsRetCode_ == SERVER_RETURN_SUCCESS)
        {
            emit allConfigsReceived(SERVER_RETURN_SUCCESS);
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}
