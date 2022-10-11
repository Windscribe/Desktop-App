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

namespace failover {

Failover::Failover(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController, const QString &nameForLog) :
    IFailover(parent), nameForLog_(nameForLog)
{
    // Creating all failovers in the order of their application
    WS_ASSERT(HardcodedSettings::instance().serverDomains().size() >= 2);
    WS_ASSERT(HardcodedSettings::instance().dynamicDomainsUrls().size() >= 2);
    WS_ASSERT(HardcodedSettings::instance().dynamicDomains().size() >= 1);

    // Hardcoded Default Domain Endpoint
    BaseFailover *failover = new HardcodedDomainFailover(this, HardcodedSettings::instance().serverDomains().at(0));
    // for the first failover, we make the DirectConnection for others the QueuedConnection
    // since the first domain is immediately set in the reset() function
    connect(failover, &BaseFailover::finished, this, &Failover::onFailoverFinished, Qt::DirectConnection);
    failovers_ << failover;

    // Don't use other failovers for the staging functionality, as the hashed domains will hit the production environment.
    if (!AppVersion::instance().isStaging()) {
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
    }

    reset();
}

QString Failover::currentHostname() const
{
    if (apiResolutionSettings_.getIsAutomatic() || apiResolutionSettings_.getManualIp().isEmpty()) {
        if (curFailoverInd_ < failovers_.size() && !curFailoverHostnames_.isEmpty() && cutFaiolverHostnameInd_ < curFailoverHostnames_.size()) {
            return curFailoverHostnames_[cutFaiolverHostnameInd_];
        }
    } else {
        return apiResolutionSettings_.getManualIp();
    }
    return QString();
}

void Failover::reset()
{
    qCDebug(LOG_FAILOVER) << QString("Failover[%1] reset").arg(nameForLog_);

    // Initialize the state to the first failover
    curFailoverInd_ = -1;
    curFailoverHostnames_.clear();
    Failover::getNextHostname(false);
}

void Failover::getNextHostname(bool bIgnoreSslErrors)
{
    if (apiResolutionSettings_.getIsAutomatic() || apiResolutionSettings_.getManualIp().isEmpty()) {
        WS_ASSERT(!isFailoverInProgress_);
        bIgnoreSslErrors_ = bIgnoreSslErrors;

        if (!curFailoverHostnames_.isEmpty() && cutFaiolverHostnameInd_ < (curFailoverHostnames_.size() - 1)) {
            cutFaiolverHostnameInd_++;
            QTimer::singleShot(0, [this]() {
                qCDebug(LOG_FAILOVER) <<  QString("Failover[%1]").arg(nameForLog_) << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[cutFaiolverHostnameInd_].left(3);
                emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[cutFaiolverHostnameInd_]);
            });
            return;
        }

        if (curFailoverInd_ >= (failovers_.size() - 1)) {
            QTimer::singleShot(0, [this]() {
                qCDebug(LOG_FAILOVER) << QString("Failover[%1]").arg(nameForLog_) << FailoverRetCode::kFailed;
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
            qCDebug(LOG_FAILOVER) <<  QString("Failover[%1]").arg(nameForLog_) << FailoverRetCode::kSuccess <<  "manualIP" << apiResolutionSettings_.getManualIp();
            emit nextHostnameAnswer(FailoverRetCode::kSuccess, apiResolutionSettings_.getManualIp());
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
    cutFaiolverHostnameInd_ = 0;

    if (retCode == FailoverRetCode::kSuccess) {
        WS_ASSERT(curFailoverHostnames_.size() >= 1);
        qCDebug(LOG_FAILOVER) << QString("Failover[%1]").arg(nameForLog_) << FailoverRetCode::kSuccess <<  failovers_[curFailoverInd_]->name() << curFailoverHostnames_[cutFaiolverHostnameInd_].left(3);
        emit nextHostnameAnswer(FailoverRetCode::kSuccess, curFailoverHostnames_[cutFaiolverHostnameInd_]);
    }
    else if (retCode == FailoverRetCode::kSslError) {
        qCDebug(LOG_FAILOVER) << QString("Failover[%1]").arg(nameForLog_) <<  failovers_[curFailoverInd_]->name() << FailoverRetCode::kSslError;
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


} // namespace failover
