#pragma once

#include "ihelperbackend.h"
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

    static constexpr int kMaxWaitHelper = 5000;

    boost::asio::io_service io_service_;
    boost::asio::local::stream_protocol::endpoint ep_;
    boost::scoped_ptr<boost::asio::local::stream_protocol::socket> socket_;
    QMutex mutexSocket_;
    QElapsedTimer reconnectElapsedTimer_;

    void connectHandler(const boost::system::error_code &ec);

    bool sendCmdToHelper(int cmdId, const std::string &data);
    bool readAnswer(std::string &answer);

    boost::system::error_code safeRead(void *data, size_t size);
    boost::system::error_code safeWrite(const void *data, size_t size);
};
