#pragma once

#include <netlistmgr.h>
#include <string>
#include <boost/asio.hpp>

#include "../../common/helper_commands.h"

class NetworkCategoryManager {
public:
    static NetworkCategoryManager& instance();
    bool setCategory(const std::wstring& networkName, NetworkCategory category);

private:
    static const int kMaxAttempts = 10;
    static const int kRetryDelayMs = 250;

    NetworkCategoryManager();
    ~NetworkCategoryManager();
    NetworkCategoryManager(const NetworkCategoryManager&) = delete;
    NetworkCategoryManager& operator=(const NetworkCategoryManager&) = delete;

    bool setCategoryInternal(const std::wstring& networkName, NetworkCategory category);

    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    std::thread io_thread_;
    std::atomic<bool> cancel_;
};
