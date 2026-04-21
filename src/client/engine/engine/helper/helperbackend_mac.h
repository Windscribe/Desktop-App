#pragma once

#include "ihelperbackend.h"
#include <atomic>
#include <QMutex>
#include <dispatch/dispatch.h>
#include <xpc/xpc.h>
#include <spdlog/spdlog.h>

// Mac implementation of IHelperBackend, thread safe
// This class is shared by the mac installer, so logging is done via spdlog
class HelperBackend_mac : public IHelperBackend
{
    Q_OBJECT
public:
    explicit HelperBackend_mac(QObject *parent, spdlog::logger *logger);
    ~HelperBackend_mac() override;

    void startInstallHelper(bool bForceDeleteOld = false) override;
    State currentState() const override;
    bool reinstallHelper() override;

    std::string sendCmd(int cmdId, const std::string &data) override;

private:
    std::atomic<State> curState_ = State::kInit;
    mutable QMutex mutex_;
    dispatch_queue_t queue_ = nullptr;
    xpc_connection_t connection_ = nullptr;
    spdlog::logger *logger_;
};
