#pragma once

#include "ifailovercontainer.h"

namespace failover {

// Container containing all failovers, more details: https://www.notion.so/windscribe/Client-Control-Plane-Failover-e4a5a0d1cf1045c99a9789db782fb19e
// It is ordered according to the documentation and has the current position.
// Some failover algorithms require IConnectStateController, in order to ensure the atomicity of the execution of a network request
// in the connected state or disconnected VPN state.In other words, a network request that failed when switching the VPN, for example
// from the connected state to the disconnected state, is not considered a failed attempt.

// Failovers are loaded into memory as it is used and deleted from memory when not needed.

class FailoverContainer : public IFailoverContainer
{
    Q_OBJECT
public:
    explicit FailoverContainer(QObject *parent, NetworkAccessManager *networkAccessManager);

    void reset() override;
    QSharedPointer<BaseFailover> currentFailover(int *outInd = nullptr) override;
    bool gotoNext() override;
    QSharedPointer<BaseFailover> failoverById(const QString &failoverUniqueId) override;
    int count() const override;

private:
    NetworkAccessManager *networkAccessManager_;
    QVector<QString> failovers_;
    QSharedPointer<BaseFailover> currentFailover_;
    int curFailoverInd_ = 0;

    void resetImpl();
};

} // namespace failover
