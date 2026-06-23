#pragma once

#include "ihelperbackend.h"
#include <atomic>
#include <QMutex>
#include "utils/boost_includes.h"

// Linux implementation of IHelperBackend, thread safe
class HelperBackend_linux : public IHelperBackend
{
    Q_OBJECT
public:
    explicit HelperBackend_linux(QObject *parent = NULL);
    ~HelperBackend_linux() override;

    void startInstallHelper(bool bForceDeleteOld = false) override;
    State currentState() const override;
    bool reinstallHelper() override;

    std::string sendCmd(int cmdId, const std::string &data) override;

protected:
    void run() override;

private:
    State curState_ = State::kInit;
    mutable QMutex mutex_;

    // Only the execution context for socket_; its event loop is not run (I/O is synchronous).
    boost::asio::io_context io_context_;
    boost::scoped_ptr<boost::asio::local::stream_protocol::socket> socket_;
    std::atomic<bool> stopRequested_{false};

    bool sendCmdToHelper(int cmdId, const std::string &data);
    bool readAnswer(std::string &answer);

    boost::system::error_code safeRead(void *data, size_t size);
    boost::system::error_code safeWrite(const void *data, size_t size);
};
