#pragma once

#include <string>
#include "scapix_object.h"

namespace wsnet {

// Advanced parameters that affect the functioning of some parts of the library
class WSNetAdvancedParameters : public scapix_object<WSNetAdvancedParameters>
{
public:
    virtual ~WSNetAdvancedParameters() {}

    virtual void setAPIExtraTLSPadding(bool isEnabled) = 0;
    virtual bool isAPIExtraTLSPadding() const = 0;

    virtual void setIgnoreCountryOverride(bool isIgnore) = 0;
    virtual bool isIgnoreCountryOverride() const = 0;

    // pass an empty string if don't need to override the country
    virtual void setCountryOverrideValue(const std::string &country) = 0;
    virtual std::string countryOverrideValue() const = 0;

    virtual void setLogApiResponce(bool isEnabled) = 0;
    virtual bool isLogApiResponce() const = 0;
};

} // namespace wsnet
