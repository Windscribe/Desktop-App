#include "ipv6controller_mac.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

void Ipv6Controller_mac::setHelper(IHelper *helper)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);
}

void Ipv6Controller_mac::disableIpv6()
{
    if (!bIsDisabled_) {
        WS_ASSERT(helper_ != NULL);
        qCDebug(LOG_BASIC) << "Disable IPv6 for all network interfaces";
        helper_->setIpv6Enabled(false);

        bIsDisabled_ = true;
    }
}

void Ipv6Controller_mac::restoreIpv6()
{
    if (bIsDisabled_) {

        WS_ASSERT(helper_ != NULL);
        qCDebug(LOG_BASIC) << "Restore IPv6 for all network interfaces";
        helper_->setIpv6Enabled(true);

        bIsDisabled_ = false;
    }
}

Ipv6Controller_mac::Ipv6Controller_mac() : bIsDisabled_(false), helper_(NULL)
{

}

Ipv6Controller_mac::~Ipv6Controller_mac()
{
    WS_ASSERT(bIsDisabled_ == false);
}