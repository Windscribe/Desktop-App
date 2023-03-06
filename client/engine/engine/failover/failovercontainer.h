#pragma once

#include "ifailovercontainer.h"

namespace failover {

// Container containing all failovers, more details: https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover.
// It is ordered according to the documentation and has the current position.
// Some failover algorithms require IConnectStateController, in order to ensure the atomicity of the execution of a network request
// in the connected state or disconnected VPN state.In other words, a network request that failed when switching the VPN, for example
// from the connected state to the disconnected state, is not considered a failed attempt.

class FailoverContainer : public IFailoverContainer
{
    Q_OBJECT
public:
    explicit FailoverContainer(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController);

    void reset() override;
    BaseFailover *currentFailover(int *outInd = nullptr) override;
    bool gotoNext() override;
    BaseFailover *failoverById(const QString &failoverUniqueId) override;
    int count() const override;

private:
    QVector<BaseFailover *> failovers_;
    int curFailoverInd_ = 0;
};

} // namespace failover
