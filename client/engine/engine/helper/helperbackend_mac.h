#pragma once

#include "ihelperbackend.h"
#include <QMutex>
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
    State curState_ = State::kInit;
    mutable QMutex mutex_;
    xpc_connection_t connection_;
    spdlog::logger *logger_;
};
