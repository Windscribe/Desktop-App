#pragma once
#include <memory>
#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <reproc++/reproc.hpp>

namespace wsnet {

typedef std::function<void(int exitCode, const std::string &output)> ProcessManagerCallback;

// Simple process manager. Allows you to start a process and set a callback function that is called after the process is finished.
// Based on reproc (Redirected Process) cross-platform C/C++ library
// In destructor kills all running processes
// Thread safe
class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

    void execute(const std::vector<std::string> &args, ProcessManagerCallback callback);

private:
    std::mutex mutex_;
    std::thread checkThread_;
    std::condition_variable condition_;
    std::atomic_bool finish_ = false;

    struct ProcessData
    {
        std::unique_ptr<reproc::process> process;
        ProcessManagerCallback callback;
    };

    std::vector<std::unique_ptr<ProcessData>> processes_;

    void checkTread();
};

} // namespace wsnet
