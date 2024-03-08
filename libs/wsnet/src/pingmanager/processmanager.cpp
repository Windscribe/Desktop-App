#include "processmanager.h"
#include <spdlog/spdlog.h>
#include <reproc/reproc.h>
#include <reproc++/drain.hpp>

namespace wsnet {

ProcessManager::ProcessManager()
{
    checkThread_ = std::thread(std::bind(&ProcessManager::checkTread, this));
}

ProcessManager::~ProcessManager()
{
    finish_ = true;
    condition_.notify_all();
    checkThread_.join();

    std::lock_guard locker(mutex_);
    for (auto &it : processes_) {
        it->process->kill();
    }
}

void ProcessManager::execute(const std::vector<std::string> &args, ProcessManagerCallback callback)
{
    auto processData = std::make_unique<ProcessData>();
    using namespace  std::placeholders;
    processData->process = std::make_unique<reproc::process>();
    std::error_code ec = processData->process->start(args);
    if (ec) {
        spdlog::error("Cannot start a process: {}, error: {}", fmt::join(args, " "), ec.value());
        return;
    }
    processData->callback = callback;

    std::lock_guard locker(mutex_);
    processes_.push_back(std::move(processData));
    condition_.notify_all();
}

void ProcessManager::checkTread()
{
    while (!finish_) {

        {
            std::unique_lock<std::mutex> locker(mutex_);
            condition_.wait(locker, [this]{ return !processes_.empty() || finish_; });
        }

        if (finish_)
            return;

        {
            std::unique_lock<std::mutex> locker(mutex_);
            auto it = processes_.begin();
            while(it != processes_.end()) {

                std::error_code ec;
                int status = 0;
                std::tie(status, ec) = (*it)->process->wait(std::chrono::milliseconds(0));
                if (status != REPROC_ETIMEDOUT) {

                    std::string output;
                    reproc::sink::string sink(output);
                    ec = reproc::drain(*(*it)->process, sink, reproc::sink::null);
                    if (!ec)
                        (*it)->callback(status, output);
                    else {
                        spdlog::error("Cannot drain a process, error: {}", ec.value());
                        (*it)->callback(-1, output);
                    }
                    it = processes_.erase(it);
                } else {
                    it++;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace wsnet
