#pragma once

#include "failover.h"

namespace server_api {

enum class FailoverState { kUnknown, kReady, kFailed };

// ServerAPI helper class. Just adds a failover state to the Failover class
class FailoverWithState : public Failover
{
    Q_OBJECT
public:
    explicit FailoverWithState(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController) :
        Failover(parent, networkAccessManager, connectStateController),
        state_(FailoverState::kUnknown)
    {}

    FailoverState state() const { return state_; }
    void setState(FailoverState state) { state_ = state; }

private:
    FailoverState state_;
};

} // namespace server_api
