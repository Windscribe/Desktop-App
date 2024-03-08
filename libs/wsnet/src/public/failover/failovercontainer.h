#pragma once

#include "ifailovercontainer.h"
#include <map>

#define FAILOVER_CONTAINER_PUBLIC  1
#define FAILOVER_DEFAULT_HARDCODED               "300fa426-4640-4a3f-b95c-1f0277462358"

namespace wsnet {

class FailoverContainer : public IFailoverContainer
{
public:
    explicit FailoverContainer(WSNetHttpNetworkManager *httpNetworkManager);

    int count() const override;

    // return first failover
    std::unique_ptr<BaseFailover> first() override;

    // return next failover after failoverUniqueId or null if not found
    std::unique_ptr<BaseFailover> next(const std::string &failoverUniqueId) override;

    // return failover by unique identifier or null if not found
    std::unique_ptr<BaseFailover> failoverById(const std::string &failoverUniqueId, int *outInd = nullptr) override;

private:
    WSNetHttpNetworkManager *httpNetworkManager_;
    std::vector<std::string> failovers_;
    std::map<std::string, int> map_;        // map failoverId to index in failovers_
};

} // namespace wsnet
