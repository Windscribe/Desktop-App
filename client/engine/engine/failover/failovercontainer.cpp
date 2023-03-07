#include "failovercontainer.h"

#include "failovers/accessipsfailover.h"
#include "failovers/dgafailover.h"
#include "failovers/dynamicdomainfailover.h"
#include "failovers/echfailover.h"
#include "failovers/hardcodeddomainfailover.h"
#include "failovers/randomdomainfailover.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

#define USE_NEW_DGA 1
#define USE_OLD_RANDOM_DOMAIN_GENERATION 1

namespace failover {

FailoverContainer::FailoverContainer(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController) :
    IFailoverContainer(parent)
{
    // Creating all failovers in the order of their application
    // Attention: do not change the unique identifiers of failover. By adding a new failover generate a new unique identifier

    // Hardcoded Default Domain Endpoint
    BaseFailover *failover = new HardcodedDomainFailover(this, "300fa426-4640-4a3f-b95c-1f0277462358", HardcodedSettings::instance().serverDomains().at(0));
    failovers_ << failover;

    // ECH
    failover = new EchFailover(this, "3e60e3d5-d379-46cc-a9a0-d9f04f47999a", networkAccessManager, "https://1.0.0.1/dns-query", "echconfig001.windscribe.dev", "ech-public-test.windscribe.dev");
    failovers_ << failover;

    // Don't use other failovers for the staging functionality, as the hashed domains will hit the production environment.
    if (!AppVersion::instance().isStaging()) {

        // Hardcoded Backup Domain Endpoint
        if (HardcodedSettings::instance().serverDomains().count() > 1) {
            failover = new HardcodedDomainFailover(this, "83e64a18-31bb-4d26-956a-ade58a5df0b9", HardcodedSettings::instance().serverDomains().at(1));
            failovers_ << failover;
        }

#ifdef USE_NEW_DGA
        // DGA
        failover = new DgaFailover(this, "c7e95d4a-ac69-4ff2-b4c0-c4e9d648b758");
        failovers_ << failover;
#endif

        // Dynamic Domains
        if (HardcodedSettings::instance().dynamicDomainsUrls().count() > 0) {
            WS_ASSERT(HardcodedSettings::instance().dynamicDomainsUrls().count() == 2);
            WS_ASSERT(HardcodedSettings::instance().dynamicDomains().count() == 1);

            failover = new DynamicDomainFailover(this, "20846580-b8fc-418b-9202-0af1fdbd90b9", networkAccessManager, HardcodedSettings::instance().dynamicDomainsUrls().at(0), HardcodedSettings::instance().dynamicDomains().at(0));
            failovers_ << failover;

            failover = new DynamicDomainFailover(this, "0555eaa2-265c-475a-bebb-fb6e65efd4af", networkAccessManager, HardcodedSettings::instance().dynamicDomainsUrls().at(1), HardcodedSettings::instance().dynamicDomains().at(0));
            failovers_ << failover;
        }

#ifdef USE_OLD_RANDOM_DOMAIN_GENERATION
        // Procedurally Generated Domain Endpoint
        failover = new RandomDomainFailover(this, "7edd0a41-1ffd-4224-a2b1-809c646a918b");
        failovers_ << failover;
#endif
        if (HardcodedSettings::instance().apiIps().count() > 0) {
            WS_ASSERT(HardcodedSettings::instance().apiIps().count() == 2);
            // making the order of IPs random
            QVector< QPair<QString, QString> > vec;
            vec << qMakePair(HardcodedSettings::instance().apiIps()[0], "5e1f1984-904d-46de-a272-64b02ff6a9d9");
            vec << qMakePair(HardcodedSettings::instance().apiIps()[1], "8d6b6c81-210f-4c69-95d2-1a983b53e895");
            vec = Utils::randomizeList< QVector< QPair<QString, QString> > >(vec);
            for (const auto & ip : vec) {
                failover = new AccessIpsFailover(this, ip.second, networkAccessManager, ip.first);
                failovers_ << failover;
            }
        }
    }
}

void FailoverContainer::reset()
{
    curFailoverInd_ = 0;
}

BaseFailover *FailoverContainer::currentFailover(int *outInd /*= nullptr*/)
{
    WS_ASSERT(curFailoverInd_ >= 0 && curFailoverInd_ < failovers_.size());
    if (outInd)
        *outInd = curFailoverInd_;
    return failovers_[curFailoverInd_];
}

bool FailoverContainer::gotoNext()
{
    if (curFailoverInd_ <  (failovers_.size() - 1)) {
        curFailoverInd_++;
        return true;
    } else {
        return false;
    }
}

BaseFailover *FailoverContainer::failoverById(const QString &failoverUniqueId)
{
    for (int i = 0; i < failovers_.size(); ++i)
        if (failovers_[i]->uniqueId() == failoverUniqueId)
            return failovers_[i];
    return nullptr;
}

int FailoverContainer::count() const
{
    return failovers_.count();
}

} // namespace failover
