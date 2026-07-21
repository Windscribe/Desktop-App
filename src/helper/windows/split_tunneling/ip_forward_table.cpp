#include "ip_forward_table.h"

#include <spdlog/spdlog.h>

IpForwardTable::IpForwardTable() : pIpForwardTable_(nullptr)
{
    DWORD dwStatus = GetIpForwardTable2(AF_UNSPEC, &pIpForwardTable_);
    if (dwStatus != NO_ERROR) {
        spdlog::error("IpForwardTable: GetIpForwardTable2 failed with error: {}", dwStatus);
        pIpForwardTable_ = nullptr;
        return;
    }
}

IpForwardTable::~IpForwardTable()
{
    if (pIpForwardTable_) {
        FreeMibTable(pIpForwardTable_);
        pIpForwardTable_ = nullptr;
    }
}

ULONG IpForwardTable::count() const
{
    return pIpForwardTable_ ? pIpForwardTable_->NumEntries : 0;
}

const MIB_IPFORWARD_ROW2 *IpForwardTable::getByIndex(ULONG ind) const
{
    if (pIpForwardTable_ && ind < pIpForwardTable_->NumEntries)
        return &pIpForwardTable_->Table[ind];
    return nullptr;
}
