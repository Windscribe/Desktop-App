#pragma once

#include <QDateTime>
#include "imacaddresscontroller.h"
#include "engine/helper/ihelper.h"

class INetworkDetectionManager;

class MacAddressController_linux : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper);
    ~MacAddressController_linux() override;

    void initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;
};
