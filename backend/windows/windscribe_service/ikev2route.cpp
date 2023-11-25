#include "all_headers.h"
#include "ikev2route.h"
#include "adapters_info.h"
#include "ip_address/ip4_address_and_mask.h"
#include "logger.h"

#pragma comment(lib, "Ntdll.lib")

bool IKEv2Route::addRouteForIKEv2()
{
    AdaptersInfo ai;
    IF_INDEX ifIndex;
    std::wstring ip;

    if (ai.getWindscribeIkev2AdapterInfo(ifIndex, ip))
    {
        MIB_IPINTERFACE_ROW interfaceRow = { 0 };
        interfaceRow.Family = AF_INET;
        interfaceRow.InterfaceIndex = ifIndex;
        if (GetIpInterfaceEntry(&interfaceRow) == NO_ERROR)
        {
            // add route
            MIB_IPFORWARDROW row = { 0 };
            row.dwForwardDest = 0;
            row.dwForwardMask = 0;
            row.dwForwardPolicy = 0;
            row.dwForwardNextHop = Ip4AddressAndMask(ip.c_str()).ipNetworkOrder();
            row.dwForwardIfIndex = ifIndex;
            row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
            row.dwForwardProto = MIB_IPPROTO_NETMGMT;
            row.dwForwardAge = INFINITE;
            row.dwForwardMetric1 = interfaceRow.Metric;

            DWORD status = CreateIpForwardEntry(&row);
            return status == NO_ERROR;
        }
        else
        {
            Logger::instance().out(L"IKEv2Route::addRouteForIKEv2(), GetIpInterfaceEntry() failed");
        }
    }
    else
    {
        Logger::instance().out(L"IKEv2Route::addRouteForIKEv2(), ai.getWindscribeIkev2AdapterInfo() failed");
    }
    return false;
}