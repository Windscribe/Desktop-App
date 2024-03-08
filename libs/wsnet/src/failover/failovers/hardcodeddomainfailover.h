#pragma once

#include "../basefailover.h"
#include "utils/utils.h"

namespace wsnet {

class HardcodedDomainFailover : public BaseFailover
{

public:
    explicit HardcodedDomainFailover(const std::string &uniqueId, const std::string &domain) : BaseFailover(uniqueId), domain_(domain) {}

    bool getData(bool /*bIgnoreSslErrors*/, std::vector<FailoverData> &data, FailoverCallback /*callback*/) override
    {
        data.clear();
        data.push_back(FailoverData(domain_));
        return true;
    }

    std::string name() const override
    {
        // the domain name has been reduced to 3 characters for log security
        return "hrd: " + utils::leftSubStr(domain_, 3);
    }

private:
    std::string domain_;
};

} // namespace wsnet
