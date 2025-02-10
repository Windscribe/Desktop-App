#pragma once

#include "fwpm_wrapper.h"

class Ipv6Firewall
{
public:
    static Ipv6Firewall &instance(FwpmWrapper *wrapper = nullptr)
    {
        static Ipv6Firewall ip6f(*wrapper);
        return ip6f;
    }

    void release();

    void enableIPv6();
    void disableIPv6();

private:
    explicit Ipv6Firewall(FwpmWrapper &fwpmWrapper);

    FwpmWrapper &fwpmWrapper_;
    GUID subLayerGUID_;
    void addFilters(HANDLE engineHandle);
};
