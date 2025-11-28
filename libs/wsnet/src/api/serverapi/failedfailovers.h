#pragma once
#include <set>
#include "failoverdata.h"

namespace wsnet {

// Helper class used by ServerAPI and RequestExecuterViaFailover to manage failed domains (FailoverData objects)
class FailedFailovers
{
public:
    void add(const FailoverData &failoverData)
    {
        failedFailovers_.insert(failoverData);
    }
    bool isContains(const FailoverData &failoverData) const
    {
        return failedFailovers_.find(failoverData) != failedFailovers_.end();
    }
    void clear()
    {
        failedFailovers_.clear();
    }

private:
    std::set<FailoverData> failedFailovers_;
};

} // namespace wsnet
