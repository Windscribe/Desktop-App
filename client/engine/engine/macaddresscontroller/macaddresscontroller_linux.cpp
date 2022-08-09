#include "macaddresscontroller_linux.h"

MacAddressController_linux::MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper) : IMacAddressController (parent)
{
    Q_UNUSED(ndManager);
    Q_UNUSED(helper);
}

MacAddressController_linux::~MacAddressController_linux()
{
}

void MacAddressController_linux::initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    Q_UNUSED(macAddrSpoofing);
}

void MacAddressController_linux::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    Q_UNUSED(macAddrSpoofing);
}
