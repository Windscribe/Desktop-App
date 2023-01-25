#include "failover.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <QTimer>

#include "failovers/hardcodeddomainfailover.h"
#include "failovers/dynamicdomainfailover.h"
#include "failovers/randomdomainfailover.h"
#include "failovers/accessipsfailover.h"
#include "failovers/dgafailover.h"
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"

#define USE_NEW_DGA 1
#define USE_OLD_RANDOM_DOMAIN_GENERATION 1

namespace failover {

Failover::Failover(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController) :
    IFailover(parent)
{
    // Creating all failovers in the order of their application

    // Hardcoded Default Domain Endpoint
    BaseFailover *failover = new HardcodedDomainFailover(this, HardcodedSettings::instance().serverDomains().at(0));
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
    failovers_ << failover;

    // Don't use other failovers for the staging functionality, as the hashed domains will hit the production environment.
    if (!AppVersion::instance().isStaging()) {
        // Hardcoded Backup Domain Endpoint
        for (int i = 1; i < HardcodedSettings::instance().serverDomains().count(); ++i) {
            failover = new HardcodedDomainFailover(this, HardcodedSettings::instance().serverDomains().at(i));
            connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
            failovers_ << failover;
        }

#ifdef USE_NEW_DGA
        // DGA
        failover = new DgaFailover(this);
        connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
        failovers_ << failover;
#endif

        // Dynamic Domains
        for (int i = 0; i < HardcodedSettings::instance().dynamicDomainsUrls().count(); ++i) {
            failover = new DynamicDomainFailover(this, networkAccessManager, HardcodedSettings::instance().dynamicDomainsUrls().at(i), HardcodedSettings::instance().dynamicDomains().at(0),
                                                 connectStateController);
            connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
            failovers_ << failover;
        }

#ifdef USE_OLD_RANDOM_DOMAIN_GENERATION
        // Procedurally Generated Domain Endpoint
        failover = new RandomDomainFailover(this);
        connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
        failovers_ << failover;
#endif

        // Hardcoded IP Endpoints (ApiAccessIps)
        // making the order of IPs random
        const QStringList apiIps = randomizeList(HardcodedSettings::instance().apiIps());
        for (const auto & ip : apiIps) {
            failover = new AccessIpsFailover(this, networkAccessManager, ip, connectStateController);
            connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::QueuedConnection);
            failovers_ << failover;
        }
    }
}

void Failover::reset()
{
    // important, do not call reset if the failover request is in progress
    WS_ASSERT(!isFailoverInProgress_);

    qCDebug(LOG_FAILOVER) << "Failover reset";
    // Initialize the state to the first failover
    curFailoverInd_ = -1;
    curFailoverHostnames_.clear();
    isAlreadyEmittedForManualDns_ = false;
}

void Failover::getNextHostname(bool bIgnoreSslErrors)
{
    WS_ASSERT(!isFailoverInProgress_);
    if (apiResolutionSettings_.getIsAutomatic() || apiResolutionSettings_.getManualAddress().isEmpty()) {
        bIgnoreSslErrors_ = bIgnoreSslErrors;

        if (!curFailoverHostnames_.isEmpty() && curFaiolverHostnameInd_ < (curFailoverHostnames_.size() - 1)) {
            curFaiolverHostnameInd_++;
            QTimer::singleShot(0, [this]() {
                qCDebug(LOG_FAILOVER) <<  "Failover" << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[curFaiolverHostnameInd_].left(3);
                emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[curFaiolverHostnameInd_]);
            });
            return;
        }

        if (curFailoverInd_ >= (failovers_.size() - 1)) {
            QTimer::singleShot(0, [this]() {
                qCDebug(LOG_FAILOVER) << "Failover" << FailoverRetCode::kFailed;
                emit nextHostnameAnswer(FailoverRetCode::kFailed, QString());
            });
            return;
        }

        curFailoverInd_++;
        isFailoverInProgress_ = true;
        failovers_[curFailoverInd_]->getHostnames(bIgnoreSslErrors);
        if (curFailoverInd_ >= 1)
            emit tryingBackupEndpoint(curFailoverInd_, failovers_.count() - 1);
    } else {
        QTimer::singleShot(0, [this]() {
            if (!isAlreadyEmittedForManualDns_) {
                qCDebug(LOG_FAILOVER) << "Failover" << FailoverRetCode::kSuccess <<  "manualAddress" << apiResolutionSettings_.getManualAddress();
                isAlreadyEmittedForManualDns_ = true;
                emit nextHostnameAnswer(FailoverRetCode::kSuccess, apiResolutionSettings_.getManualAddress());
            } else {
                qCDebug(LOG_FAILOVER) << "Failover" << FailoverRetCode::kFailed << "for manualAddress";
                emit nextHostnameAnswer(FailoverRetCode::kFailed, QString());
            }
        });
    }
}

void Failover::setApiResolutionSettings(const types::ApiResolutionSettings &apiResolutionSettings)
{
    apiResolutionSettings_ = apiResolutionSettings;
}

void Failover::onFailoverFinished(FailoverRetCode retCode, const QStringList &hostnames)
{
    WS_ASSERT(isFailoverInProgress_);
    isFailoverInProgress_ = false;

    curFailoverHostnames_ = randomizeList(hostnames);
    curFaiolverHostnameInd_ = 0;

    if (retCode == FailoverRetCode::kSuccess) {
        WS_ASSERT(curFailoverHostnames_.size() >= 1);
        qCDebug(LOG_FAILOVER) << "Failover" << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[curFaiolverHostnameInd_].left(3);
        emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[curFaiolverHostnameInd_]);
    }
    else if (retCode == FailoverRetCode::kConnectStateChanged) {
        // do not switch to the next filer when the connect state changes
        if (curFailoverInd_ > 0) curFailoverInd_--;

        qCDebug(LOG_FAILOVER) << "Failover" <<  failovers_[curFailoverInd_]->name() << FailoverRetCode::kConnectStateChanged;
        emit nextHostnameAnswer(FailoverRetCode::kConnectStateChanged, QString());
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


} // namespace failover
