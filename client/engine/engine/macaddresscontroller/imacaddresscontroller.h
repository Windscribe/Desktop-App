#pragma once

#include <QObject>
#include "types/macaddrspoofing.h"

class IMacAddressController : public QObject
{
    Q_OBJECT
public:
    explicit IMacAddressController(QObject *parent) : QObject(parent) {}
    virtual ~IMacAddressController() {}

    virtual void initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) = 0;
    virtual void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) = 0;

signals:
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void sendUserWarning(USER_WARNING_TYPE userWarningType);
    // MacOS only
    void macSpoofApplied();
};
