#include "processmanager.h"
#include "utils/wsnet_logger.h"

namespace wsnet {

ProcessManager::ProcessManager(boost::asio::io_context &io_context) :
    io_context_(io_context)
{
}

ProcessManager::~ProcessManager()
{
    std::lock_guard locker(mutex_);
    for (auto &it : processes_) {
        it.second->process.terminate();
    }
}

bool ProcessManager::execute(const std::string &cmd, const std::vector<std::string> &args, ProcessManagerCallback callback)
{
    try {
        std::lock_guard locker(mutex_); // NOLINT
        auto childProcess = std::make_unique<ChildProcess>();
        childProcess->callback = callback;
        childProcess->process = boost::process::v1::child(boost::process::v1::search_path(cmd), args,
            boost::process::v1::std_out >  childProcess->data,
            io_context_,
            boost::process::v1::on_exit = [this, id = curId_](int exit, std::error_code ec) {
                // on exit function handler
                std::string data;
                ProcessManagerCallback callback;
                // copy data and callback and remove an item from processes
                {
                    std::lock_guard locker(mutex_);
                    auto it = processes_.find(id);
                    if (it != processes_.end()) {
                        std::ostringstream os;
                        os << it->second->data.rdbuf();
                        data = os.str();
                        callback = it->second->callback;
                        processes_.erase(it);
                    } else {
                        assert(false);
                    }
                }
                // call callback
                callback(exit, data);
            });

        processes_[curId_++] = std::move(childProcess);

    } catch(...) {
        g_logger->error("Cannot start a process: {}", cmd);
        return false;
    }

    return true;
}

} // namespace wsnet
