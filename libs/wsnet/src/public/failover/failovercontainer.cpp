#include "failovercontainer.h"

#include "failovers/hardcodeddomainfailover.h"
#include "settings.h"
#include "utils/utils.h"

namespace wsnet {

FailoverContainer::FailoverContainer(WSNetHttpNetworkManager *httpNetworkManager) :
    httpNetworkManager_(httpNetworkManager)
{
    failovers_.push_back(FAILOVER_DEFAULT_HARDCODED);

    for (int i = 0; i < failovers_.size(); i++)
        map_[failovers_[i]] = i;
}

int FailoverContainer::count() const
{
    return (int)failovers_.size();
}

std::unique_ptr<BaseFailover> FailoverContainer::first()
{
    return failoverById(FAILOVER_DEFAULT_HARDCODED);
}

std::unique_ptr<BaseFailover> FailoverContainer::next(const std::string &failoverUniqueId)
{
    auto it = map_.find(failoverUniqueId);
    if (it == map_.end())
        return nullptr;

    // goto next
    if (it->second >= (failovers_.size() - 1))  // is that the last one?
        return nullptr;

    return failoverById(failovers_[it->second + 1]);
 }

std::unique_ptr<BaseFailover> FailoverContainer::failoverById(const std::string &failoverUniqueId, int *outInd)
{
    auto it = map_.find(failoverUniqueId);
    if (it == map_.end()) {
        return nullptr;
    }
    if (outInd) *outInd = it->second;

    Settings &settings = Settings::instance();

    if (failoverUniqueId == FAILOVER_DEFAULT_HARDCODED) {
        return std::make_unique<HardcodedDomainFailover>(FAILOVER_DEFAULT_HARDCODED, settings.primaryServerDomain());
    }

    // should not be here
    assert(false);
    return nullptr;
}

} // namespace wsnet
