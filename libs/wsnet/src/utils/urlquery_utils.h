#pragma once

#include <skyr/url.hpp>

namespace wsnet {

namespace urlquery_utils
{
    void addPlatformQueryItems(skyr::url_search_parameters &sp);
    void addAuthQueryItems(skyr::url_search_parameters &sp);
}

} // namespace wsnet
