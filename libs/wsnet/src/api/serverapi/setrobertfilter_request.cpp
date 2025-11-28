#include "setrobertfilter_request.h"
#include <skyr/url.hpp>
#include "utils/urlquery_utils.h"


namespace wsnet {

SetRobertFilterRequest::SetRobertFilterRequest(HttpMethod requestType, SubdomainType subDomainType, RequestPriority priority, const std::string &name,
        std::map<std::string, std::string> extraParams, const std::string &jsonPostData, RequestFinishedCallback callback) :
    BaseRequest(requestType, subDomainType, priority, name, extraParams, callback),
    jsonPostData_(jsonPostData)
{
}

std::string SetRobertFilterRequest::url(const std::string &domain) const
{
    auto url = skyr::url("https://" + hostname(domain, subDomainType_) + "/" + name());
    auto &sp = url.search_parameters();
    for (auto &it : extraParams_)
        if (!it.second.empty())
            sp.set(it.first, it.second);
    urlquery_utils::addPlatformQueryItems(sp);

    return url.c_str();
}

std::string SetRobertFilterRequest::postData() const
{
    return jsonPostData_;
}


} // namespace wsnet
