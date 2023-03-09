#include "failovercontainer.h"

#include "failovers/accessipsfailover.h"
#include "failovers/dgafailover.h"
#include "failovers/dynamicdomainfailover.h"
#include "failovers/echfailover.h"
#include "failovers/hardcodeddomainfailover.h"
#include "failovers/randomdomainfailover.h"
#include "utils/hardcodedsettings.h"
#include "utils/dga_library.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"


// All failovers in the order of their application by unique id
// Attention: do not change the unique identifiers of failover. By adding a new failover generate a new unique identifier
#define FAILOVER_DEFAULT_HARDCODED               "300fa426-4640-4a3f-b95c-1f0277462358"
#define FAILOVER_BACKUP_HARDCODED                "83e64a18-31bb-4d26-956a-ade58a5df0b9"

#define FAILOVER_ECH_CLOUFLARE_1                 "3e60e3d5-d379-46cc-a9a0-d9f04f47999a"
#define FAILOVER_ECH_CLOUFLARE_2                 "ee195090-f5e8-4ae0-9142-ca0961a43173"
#define FAILOVER_ECH_CLOUFLARE_3                 "ad532351-5999-4a64-859f-32a021511876"

// For these three, we  do not need to update ECH config the timeout(TTL)
#define FAILOVER_ECH_CLOUFLARE_FALLBACK_1        "7f01c68b-d06f-4522-99f3-9c31762f24a4 "
#define FAILOVER_ECH_CLOUFLARE_FALLBACK_2        "3ddc4f4c-37d3-44e7-bdba-877a5666d2ac"
#define FAILOVER_ECH_CLOUFLARE_FALLBACK_3        "da3f5ac6-897b-44be-b401-bfbceaca4ce8"

#define FAILOVER_DGA                             "c7e95d4a-ac69-4ff2-b4c0-c4e9d648b758"
#define FAILOVER_OLD_RANDOM_DOMAIN_GENERATION    "7edd0a41-1ffd-4224-a2b1-809c646a918b"

#define FAILOVER_DYNAMIC_CLOUDFLARE_1            "20846580-b8fc-418b-9202-0af1fdbd90b9"
#define FAILOVER_DYNAMIC_CLOUDFLARE_2            "d1f8d432-3ef3-4b26-864e-f3bd44fede77"
#define FAILOVER_DYNAMIC_CLOUDFLARE_3            "bd35e5d2-fbe2-4e1a-9ae6-2d641c3cf26e"

#define FAILOVER_DYNAMIC_GOOGLE_1                "0555eaa2-265c-475a-bebb-fb6e65efd4af"
#define FAILOVER_DYNAMIC_GOOGLE_2                "a84a8883-1d82-409b-8521-10d8040e9557"
#define FAILOVER_DYNAMIC_GOOGLE_3                "b08c7b45-f65b-4d7f-a424-9fb71f296957"

#define FAILOVER_ACCESS_IP_1                     "5efc039e-0937-4ad7-a82e-f4097987027e"
#define FAILOVER_ACCESS_IP_2                     "c80e1193-27ea-405a-9502-00c31a51c911"

namespace failover {

FailoverContainer::FailoverContainer(QObject *parent, NetworkAccessManager *networkAccessManager) :
    IFailoverContainer(parent),
    networkAccessManager_(networkAccessManager)
{
    // Array of all failover ids in the order of their application
    failovers_ << FAILOVER_DEFAULT_HARDCODED;

    // Don't use other failovers for the staging functionality, as the hashed domains will hit the production environment.
    if (!AppVersion::instance().isStaging()) {
        failovers_ << FAILOVER_BACKUP_HARDCODED;

        failovers_ << FAILOVER_DYNAMIC_CLOUDFLARE_1;
        failovers_ << FAILOVER_DYNAMIC_CLOUDFLARE_2;
        failovers_ << FAILOVER_DYNAMIC_CLOUDFLARE_3;

        failovers_ << FAILOVER_DYNAMIC_GOOGLE_1;
        failovers_ << FAILOVER_DYNAMIC_GOOGLE_2;
        failovers_ << FAILOVER_DYNAMIC_GOOGLE_3;

        failovers_ << FAILOVER_OLD_RANDOM_DOMAIN_GENERATION;
        failovers_ << FAILOVER_DGA;

        failovers_ << FAILOVER_ECH_CLOUFLARE_1;
        failovers_ << FAILOVER_ECH_CLOUFLARE_2;
        failovers_ << FAILOVER_ECH_CLOUFLARE_3;

        failovers_ << FAILOVER_ECH_CLOUFLARE_FALLBACK_1;
        failovers_ << FAILOVER_ECH_CLOUFLARE_FALLBACK_2;
        failovers_ << FAILOVER_ECH_CLOUFLARE_FALLBACK_3;

        // randomize access ips order
        if (Utils::generateIntegerRandom(0, 1) == 0) {
            failovers_ << FAILOVER_ACCESS_IP_1;
            failovers_ << FAILOVER_ACCESS_IP_2;
        } else {
            failovers_ << FAILOVER_ACCESS_IP_2;
            failovers_ << FAILOVER_ACCESS_IP_1;
        }
    }

    resetImpl();
}

void FailoverContainer::reset()
{
    resetImpl();
}

QSharedPointer<BaseFailover> FailoverContainer::currentFailover(int *outInd /*= nullptr*/)
{
    WS_ASSERT(curFailoverInd_ >= 0 && curFailoverInd_ < failovers_.size());
    if (outInd)
        *outInd = curFailoverInd_;
    return currentFailover_;
}

bool FailoverContainer::gotoNext()
{
    if (curFailoverInd_ <  (failovers_.size() - 1)) {
        curFailoverInd_++;
        currentFailover_ = failoverById(failovers_[curFailoverInd_]);
        return !currentFailover_.isNull();
    } else {
        return false;
    }
}

QSharedPointer<BaseFailover> FailoverContainer::failoverById(const QString &failoverUniqueId)
{
    if (failoverUniqueId == FAILOVER_DEFAULT_HARDCODED) {
        return QSharedPointer<BaseFailover>(new HardcodedDomainFailover(this, FAILOVER_DEFAULT_HARDCODED, HardcodedSettings::instance().primaryServerDomain()));
    } else if (failoverUniqueId == FAILOVER_BACKUP_HARDCODED) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new HardcodedDomainFailover(this, FAILOVER_BACKUP_HARDCODED, dga.getParameter(PAR_BACKUP_DOMAIN)));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_1) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_1, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL1), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), false));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_2) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_2, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), false));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_3) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_3, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL3), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), false));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_FALLBACK_1) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_FALLBACK_1, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL1), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), true));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_FALLBACK_2) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_FALLBACK_2, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), true));
        }
    } else if (failoverUniqueId == FAILOVER_ECH_CLOUFLARE_FALLBACK_3) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new EchFailover(this, FAILOVER_ECH_CLOUFLARE_FALLBACK_3, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL3), dga.getParameter(PAR_ECH_CONFIG_DOMAIN), dga.getParameter(PAR_ECH_DOMAIN), true));
        }
    } else if (failoverUniqueId == FAILOVER_DGA) {
        return QSharedPointer<BaseFailover>(new DgaFailover(this, FAILOVER_DGA));
    } else if (failoverUniqueId == FAILOVER_OLD_RANDOM_DOMAIN_GENERATION) {
        return QSharedPointer<BaseFailover>(new RandomDomainFailover(this, FAILOVER_OLD_RANDOM_DOMAIN_GENERATION));
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_CLOUDFLARE_1) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_CLOUDFLARE_1, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL1), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_CLOUDFLARE_2) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_CLOUDFLARE_2, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL2), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_CLOUDFLARE_3) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_CLOUDFLARE_3, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_CLOUDFLARE_URL3), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_GOOGLE_1) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_GOOGLE_1, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_GOOGLE_URL1), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_GOOGLE_2) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_GOOGLE_2, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_GOOGLE_URL2), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_DYNAMIC_GOOGLE_3) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new DynamicDomainFailover(this, FAILOVER_DYNAMIC_GOOGLE_3, networkAccessManager_, dga.getParameter(PAR_DYNAMIC_DOMAIN_GOOGLE_URL3), dga.getParameter(PAR_DYNAMIC_DOMAIN_DESKTOP)));
        }
    } else if (failoverUniqueId == FAILOVER_ACCESS_IP_1) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new AccessIpsFailover(this, FAILOVER_ACCESS_IP_1, networkAccessManager_, dga.getParameter(PAR_API_ACCESS_IP1)));
        }
    } else if (failoverUniqueId == FAILOVER_ACCESS_IP_2) {
        DgaLibrary dga;
        if (dga.load()) {
            return QSharedPointer<BaseFailover>(new AccessIpsFailover(this, FAILOVER_ACCESS_IP_2, networkAccessManager_, dga.getParameter(PAR_API_ACCESS_IP2)));
        }
    }

    return nullptr;
}

int FailoverContainer::count() const
{
    return failovers_.count();
}

void FailoverContainer::resetImpl()
{
    curFailoverInd_ = 0;
    currentFailover_ = QSharedPointer<BaseFailover>(new HardcodedDomainFailover(this, FAILOVER_DEFAULT_HARDCODED, HardcodedSettings::instance().primaryServerDomain()));
}

} // namespace failover
