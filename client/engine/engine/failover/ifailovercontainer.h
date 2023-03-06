#pragma once

#include <QObject>
#include "basefailover.h"

class NetworkAccessManager;
class IConnectStateController;

namespace failover {

// Interface for an ordered failover container
class IFailoverContainer : public QObject
{
    Q_OBJECT
public:
    explicit IFailoverContainer(QObject *parent) : QObject(parent) {}
    virtual ~IFailoverContainer() {}

     // switch current postion to the beginning
    virtual void reset() = 0;
    virtual BaseFailover *currentFailover(int *outInd = nullptr) = 0;
    // switch to the next failover or return false if it was the last one
    virtual bool gotoNext() = 0;
    // return failover by unique identifier or null if not found
    virtual BaseFailover *failoverById(const QString &failoverUniqueId) = 0;
    virtual int count() const = 0;
};

} // namespace failover
