#include "failover.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <QTimer>

#include "failovers/hardcodeddomainfailover.h"
#include "failovers/dynamicdomainfailover.h"
#include "failovers/randomdomainfailover.h"
#include "failovers/accessipsfailover.h"
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"

namespace server_api {

//FIXME: move all this to secrets?
Failover::Failover(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController) :
    QObject(parent)
{
    // Creating all failovers in the order of their application
    WS_ASSERT(HardcodedSettings::instance().serverDomains().size() >= 2);
    WS_ASSERT(HardcodedSettings::instance().dynamicDomainsUrls().size() >= 2);
    WS_ASSERT(HardcodedSettings::instance().dynamicDomains().size() >= 1);

    // Hardcoded Default Domain Endpoint
    BaseFailover *failover = new HardcodedDomainFailover(this, "deepstateplatypus.com"/*HardcodedSettings::instance().serverDomains().at(0)*/);
    // for the first failover, we make the DirectConnection for others the QueuedConnection
    // since the first domain is immediately set in the reset() function
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::DirectConnection);
    failovers_ << failover;

    // Hardcoded Backup Domain Endpoint
    failover = new HardcodedDomainFailover(this, HardcodedSettings::instance().serverDomains().at(1));
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
    failovers_ << failover;

    // Dynamic Domain Cloudflare
    failover = new DynamicDomainFailover(this, networkAccessManager, HardcodedSettings::instance().dynamicDomainsUrls().at(0), HardcodedSettings::instance().dynamicDomains().at(0),
                                         connectStateController);
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
    failovers_ << failover;

    // Dynamic Domain Google
    failover = new DynamicDomainFailover(this, networkAccessManager, HardcodedSettings::instance().dynamicDomainsUrls().at(1), HardcodedSettings::instance().dynamicDomains().at(0),
                                         connectStateController);
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
    failovers_ << failover;

    // Procedurally Generated Domain Endpoint
    failover = new RandomDomainFailover(this);
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
    failovers_ << failover;

    // Hardcoded IP Endpoints (ApiAccessIps)
    // making the order of IPs random
    const QStringList apiIps = randomizeList(HardcodedSettings::instance().apiIps());
    for (const auto & ip : apiIps) {
        failover = new AccessIpsFailover(this, networkAccessManager, ip, connectStateController);
        connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
        failovers_ << failover;
    }

    reset();
}

QString Failover::currentHostname() const
{
    if (curFailoverInd_ < failovers_.size() && !curFailoverHostnames_.isEmpty() && cutFaiolverHostnameInd_ < curFailoverHostnames_.size()) {
        qCDebug(LOG_SERVER_API) << "Failover::currentHostname:" <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[cutFaiolverHostnameInd_].left(3);
        return curFailoverHostnames_[cutFaiolverHostnameInd_];
    }
    return QString();
}

void Failover::reset()
{
    qCDebug(LOG_SERVER_API) << "Failover::reset";

    // Initialize the state to the first failover
    curFailoverInd_ = -1;
    curFailoverHostnames_.clear();
    getNextHostname(false);
    int g = 0;
}

void Failover::getNextHostname(bool bIgnoreSslErrors)
{
    WS_ASSERT(!isFailoverInProgress_);
    bIgnoreSslErrors_ = bIgnoreSslErrors;

    if (!curFailoverHostnames_.isEmpty() && cutFaiolverHostnameInd_ < (curFailoverHostnames_.size() - 1)) {
        cutFaiolverHostnameInd_++;
        QTimer::singleShot(0, [this]() {
            qCDebug(LOG_SERVER_API) << "Failover::nextHostnameAnswer:" << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[cutFaiolverHostnameInd_].left(3);
            emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[cutFaiolverHostnameInd_]);
        });
        return;
    }

    if (curFailoverInd_ >= (failovers_.size() - 1)) {
        QTimer::singleShot(0, [this]() {
            qCDebug(LOG_SERVER_API) << "Failover::nextHostnameAnswer:" << FailoverRetCode::kFailed;
            emit nextHostnameAnswer(FailoverRetCode::kFailed, QString());
        });
        return;
    }

    curFailoverInd_++;
    isFailoverInProgress_ = true;
    failovers_[curFailoverInd_]->getHostnames(bIgnoreSslErrors);
}

void Failover::onFailoverFinished(server_api::FailoverRetCode retCode, const QStringList &hostnames)
{
    WS_ASSERT(isFailoverInProgress_);
    isFailoverInProgress_ = false;

    curFailoverHostnames_ = randomizeList(hostnames);
    cutFaiolverHostnameInd_ = 0;

    if (retCode == FailoverRetCode::kSuccess) {
        WS_ASSERT(curFailoverHostnames_.size() >= 1);
        qCDebug(LOG_SERVER_API) << "Failover::nextHostnameAnswer:" << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[cutFaiolverHostnameInd_].left(3);
        emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[cutFaiolverHostnameInd_]);
    }
    else if (retCode == FailoverRetCode::kSslError) {
        qCDebug(LOG_SERVER_API) << "Failover::nextHostnameAnswer:" <<  failovers_[curFailoverInd_]->name() << FailoverRetCode::kSslError;
        emit nextHostnameAnswer(FailoverRetCode::kSslError, QString());
    }
    else if (retCode == FailoverRetCode::kFailed) {
        getNextHostname(bIgnoreSslErrors_);
    }
}

QStringList Failover::randomizeList(const QStringList &list)
{
    QStringList result = list;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle( result.begin(), result.end(), std::default_random_engine(seed));
    return result;
}


} // namespace server_api
