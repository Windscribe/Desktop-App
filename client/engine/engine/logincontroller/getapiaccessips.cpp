#include "getapiaccessips.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include "utils/utils.h"

GetApiAccessIps::GetApiAccessIps(QObject *parent, ServerAPI *serverAPI) : QObject(parent),
    serverAPI_(serverAPI)
{
    connect(serverAPI_, SIGNAL(accessIpsAnswer(SERVER_API_RET_CODE,QStringList, uint)), SLOT(onAccessIpsAnswer(SERVER_API_RET_CODE,QStringList, uint)), Qt::QueuedConnection);
    serverApiUserRole_ = serverAPI_->getAvailableUserRole();
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

void GetApiAccessIps::onAccessIpsAnswer(SERVER_API_RET_CODE retCode, const QStringList &hosts, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            emit finished(SERVER_RETURN_SUCCESS, hosts);
        }
        else if (retCode == SERVER_RETURN_PROXY_AUTH_FAILED)
        {
            emit finished(SERVER_RETURN_PROXY_AUTH_FAILED, QStringList());
        }
        else if (retCode == SERVER_RETURN_SSL_ERROR)
        {
            emit finished(SERVER_RETURN_SSL_ERROR, QStringList());
        }
        else
        {
            if (hardcodedIps_.count() > 0)
            {
                makeRequestToRandomIP();
            }
            else
            {
                emit finished(SERVER_RETURN_NETWORK_ERROR, QStringList());
            }
        }
    }
}

void GetApiAccessIps::makeRequestToRandomIP()
{
    int randomInd = Utils::generateIntegerRandom(0, hardcodedIps_.count() - 1); // random number from 0 to hardcodedIps_.count() - 1
    qCDebug(LOG_BASIC) << "Make ApiAccessIps request with "  << randomInd;
    serverAPI_->accessIps(hardcodedIps_[randomInd], serverApiUserRole_);
    hardcodedIps_.removeAt(randomInd);
}
