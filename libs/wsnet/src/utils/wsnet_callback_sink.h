#pragma once

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/synchronous_factory.h>

#include <mutex>
#include <string>

// custom extension for spdlog to pass the log string to a custom callback function
namespace wsnet {

// callbacks type
typedef std::function<void(const std::string &msg)> wsnet_log_callback;

/*
 * Trivial callback sink, gets a callback function and calls it on each log
 */
template<typename Mutex>
class wsnet_callback_sink final : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit wsnet_callback_sink(const wsnet_log_callback &callback)
        : callback_{callback}
    {}

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        formatted.push_back('\0'); // add a null terminator

        callback_(formatted.data());
    }
    void flush_() override{};

private:
    wsnet_log_callback callback_;
};

using wsnet_callback_sink_mt = wsnet_callback_sink<std::mutex>;


//
// factory functions
//
template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger> callback_logger_mt(const std::string &logger_name, const wsnet_log_callback &callback)
{
    return Factory::template create<wsnet_callback_sink_mt>(logger_name, callback);
}

} // namespace wsnet
