#ifndef IMACADDRESSCONTROLLER_H
#define IMACADDRESSCONTROLLER_H

#include <QObject>
#include "utils/protobuf_includes.h"

class IMacAddressController : public QObject
{
    Q_OBJECT
public:
    explicit IMacAddressController(QObject *parent) : QObject(parent) {}
    virtual ~IMacAddressController() {}

    virtual void initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) = 0;
    virtual void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) = 0;

signals:
    void macAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void sendUserWarning(ProtoTypes::UserWarningType userWarningType);
    void robustMacSpoofApplied();
};

#endif // IMACADDRESSCONTROLLER_H

