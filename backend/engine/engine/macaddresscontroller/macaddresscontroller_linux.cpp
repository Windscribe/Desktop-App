#include "macaddresscontroller_linux.h"

#include "utils/utils.h"
#include "utils/logger.h"

MacAddressController_linux::MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper) : IMacAddressController (parent)
{
}

MacAddressController_linux::~MacAddressController_linux()
{

}

void MacAddressController_linux::initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{

}

void MacAddressController_linux::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{

}
