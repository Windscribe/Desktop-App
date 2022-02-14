#ifndef MACADDRESSCONTROLLER_LINUX_H
#define MACADDRESSCONTROLLER_LINUX_H

#include <QDateTime>
#include "imacaddresscontroller.h"
#include "engine/helper/ihelper.h"
#include "../networkdetectionmanager/networkdetectionmanager_linux.h"

class MacAddressController_linux : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper);
    ~MacAddressController_linux() override;

    void initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;
};

#endif // MACADDRESSCONTROLLER_LINUX_H
