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
    virtual BaseFailover *currentFailover() = 0;
    // switch to the next failover or return false if it was the last one
    virtual bool gotoNext() = 0;
    // move the failover with id to top of the list and make it current
    // if such a failover is not found, then the order of the list does not change and the first failover in the list becomes current
    virtual void moveFailoverToTop(const QString &failoverUniqueId) = 0;
};

} // namespace failover
