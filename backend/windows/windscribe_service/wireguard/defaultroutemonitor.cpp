#include "../all_headers.h"
#include "../logger.h"
#include "defaultroutemonitor.h"
#include "wireguardcommunicator.h"


namespace
{
void __stdcall RouteChangeCallback(_In_ PVOID CallerContext,
                                   _In_ PMIB_IPFORWARD_ROW2 Row OPTIONAL,
                                   _In_ MIB_NOTIFICATION_TYPE /*NotificationType*/)
{
    auto *this_ = static_cast<DefaultRouteMonitor *>(CallerContext);
    if (this_ && Row && Row->DestinationPrefix.PrefixLength == 0)
        this_->checkDefaultRoutes();
}

void __stdcall IpInterfaceChangeCallback(_In_ PVOID CallerContext,
                                         _In_ PMIB_IPINTERFACE_ROW /*Row*/ OPTIONAL,
                                         _In_ MIB_NOTIFICATION_TYPE NotificationType)
{
    auto *this_ = static_cast<DefaultRouteMonitor *>(CallerContext);
    if (this_ && NotificationType == MibParameterNotification)
        this_->checkDefaultRoutes();
}

bool GetInterfaceForSocket(ADDRESS_FAMILY af, NET_LUID luid, NET_IFINDEX *lastIndex)
{
    assert(lastIndex);
    MIB_IPFORWARD_TABLE2 *table = nullptr;
    auto status = GetIpForwardTable2(af, &table);
    if (status != NO_ERROR) {
        Logger::instance().out(L"GetInterfaceForSocket: can't GetIpForwardTable2");
        return false;
    }
    ULONG lowestMetric = ~ULONG(0);
    NET_IFINDEX bestIfIndex = NET_IFINDEX_UNSPECIFIED;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const auto *row = &table->Table[i];
        if (row->DestinationPrefix.PrefixLength != 0 ||
            row->InterfaceLuid.Value == luid.Value)
            continue;
        MIB_IF_ROW2 ifrow;
        ifrow.InterfaceLuid = luid;
        status = GetIfEntry2(&ifrow);
        if (status != NO_ERROR || ifrow.OperStatus != IfOperStatusUp)
            continue;
        MIB_IPINTERFACE_ROW iprow;
        InitializeIpInterfaceEntry(&iprow);
        iprow.InterfaceLuid = luid;
        iprow.Family = af;
        status = GetIpInterfaceEntry(&iprow);
        if (status != NO_ERROR)
            continue;
        const auto currentMetric = row->Metric + iprow.Metric;
        if (currentMetric  < lowestMetric) {
            lowestMetric = currentMetric;
            bestIfIndex = row->InterfaceIndex;
        }
    }
    FreeMibTable(table);
    if (lastIndex) {
        if (*lastIndex == bestIfIndex)
            return false;
        *lastIndex = bestIfIndex;
    }
    return true;
}
}  // namespace

DefaultRouteMonitor::DefaultRouteMonitor(std::shared_ptr<WireGuardCommunicator> comm,
                                         NET_LUID adapterLuid)
    : comm_(comm), adapterLuid_(adapterLuid),
      lastIfIndex4_(~NET_IFINDEX(0)), lastIfIndex6_(~NET_IFINDEX(0)),
      routeChangeCallback_(nullptr), ipInterfaceChangeCallback_(nullptr)
{
}

DefaultRouteMonitor::~DefaultRouteMonitor()
{
    stop();
}

bool DefaultRouteMonitor::start()
{
    auto status = NotifyRouteChange2(AF_UNSPEC, RouteChangeCallback, this,
        FALSE, &routeChangeCallback_);
    if (status != NO_ERROR)
        return false;
    status = NotifyIpInterfaceChange(AF_UNSPEC, IpInterfaceChangeCallback, this,
        FALSE, &ipInterfaceChangeCallback_);
    if (status != NO_ERROR)
        return false;
    return checkDefaultRoutes();
}

void DefaultRouteMonitor::stop()
{
    if (routeChangeCallback_) {
        CancelMibChangeNotify2(routeChangeCallback_);
        routeChangeCallback_ = nullptr;
    }
    if (ipInterfaceChangeCallback_) {
        CancelMibChangeNotify2(ipInterfaceChangeCallback_);
        ipInterfaceChangeCallback_ = nullptr;
    }
}

bool DefaultRouteMonitor::checkDefaultRoutes()
{
    auto if4 = WireGuardCommunicator::SOCKET_INTERFACE_KEEP;
    auto if6 = WireGuardCommunicator::SOCKET_INTERFACE_KEEP;
    bool if6blackhole = false;

    if (GetInterfaceForSocket(AF_INET, adapterLuid_, &lastIfIndex4_)) {
        if4 = lastIfIndex4_;
    }
    if (GetInterfaceForSocket(AF_INET6, adapterLuid_, &lastIfIndex6_)) {
        if6 = lastIfIndex6_;
        if6blackhole = lastIfIndex6_ == NET_IFINDEX_UNSPECIFIED;
    }
    return comm_->bindSockets(if4, if6, if6blackhole);
}
