#include "getapiaccessips.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/serverapi/requests/accessipsrequest.h"

GetApiAccessIps::GetApiAccessIps(QObject *parent, server_api::ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI)
{
}

void GetApiAccessIps::get()
{
    hardcodedIps_ = HardcodedSettings::instance().apiIps();
    if (hardcodedIps_.count() > 0) {
        makeRequestToRandomIP();
    }
    else {
        emit finished(SERVER_RETURN_NETWORK_ERROR, QStringList());
    }
}

void GetApiAccessIps::onAccessIpsAnswer()
{
    QSharedPointer<server_api::AcessIpsRequest> request(static_cast<server_api::AcessIpsRequest *>(sender()), &QObject::deleteLater);

    if (request->retCode() == SERVER_RETURN_SUCCESS)
        emit finished(SERVER_RETURN_SUCCESS, request->hosts());
    else if (request->retCode() == SERVER_RETURN_PROXY_AUTH_FAILED)
        emit finished(SERVER_RETURN_PROXY_AUTH_FAILED, QStringList());
    else if (request->retCode() == SERVER_RETURN_SSL_ERROR)
        emit finished(SERVER_RETURN_SSL_ERROR, QStringList());
    else  {
        if (hardcodedIps_.count() > 0)
            makeRequestToRandomIP();
        else
            emit finished(SERVER_RETURN_NETWORK_ERROR, QStringList());
    }
}

void GetApiAccessIps::makeRequestToRandomIP()
{
    int randomInd = Utils::generateIntegerRandom(0, hardcodedIps_.count() - 1); // random number from 0 to hardcodedIps_.count() - 1
    qCDebug(LOG_BASIC) << "Make ApiAccessIps request with "  << randomInd;
    server_api::BaseRequest *request = serverAPI_->accessIps(hardcodedIps_[randomInd]);
    connect(request, &server_api::BaseRequest::finished, this, &GetApiAccessIps::onAccessIpsAnswer);
    hardcodedIps_.removeAt(randomInd);
}
