#pragma once

#include "basefailover.h"

namespace wsnet {

// Interface for an ordered failover container
class IFailoverContainer
{
public:
    virtual ~IFailoverContainer() {}

    virtual int count() const = 0;

    // return first failover
    virtual std::unique_ptr<BaseFailover> first() = 0;

    // return next failover after failoverUniqueId or null if not found
    virtual std::unique_ptr<BaseFailover> next(const std::string &failoverUniqueId) = 0;

    // return failover by unique identifier or null if not found
    virtual std::unique_ptr<BaseFailover> failoverById(const std::string &failoverUniqueId, int *outInd = nullptr) = 0;
};

} // namespace failover
