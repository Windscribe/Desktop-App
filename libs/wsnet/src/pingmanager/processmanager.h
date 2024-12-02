#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace wsnet {

typedef std::function<void(int exitCode, const std::string &output)> ProcessManagerCallback;

// Simple process manager. Allows you to start a process and set a callback function that is called after the process is finished.
// Thread safe
class ProcessManager
{
public:
    ProcessManager(boost::asio::io_context &io_context);
    ~ProcessManager();

    bool execute(const std::string &cmd, const std::vector<std::string> &args, ProcessManagerCallback callback);

private:
    boost::asio::io_context &io_context_;

    struct ChildProcess
    {
        boost::process::child process;
        boost::process::ipstream data;
        ProcessManagerCallback callback;
    };

    std::mutex mutex_;
    uint64_t curId_ = 0;
    std::unordered_map<uint64_t, std::unique_ptr<ChildProcess>> processes_;
};

} // namespace wsnet
