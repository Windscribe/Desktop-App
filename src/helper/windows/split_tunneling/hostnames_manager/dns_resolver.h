#pragma once

#include <string>
#include <map>
#include <optional>
#include <wsnet/WSNet.h>
#include <boost/asio.hpp>

// measuring time in ms helper
template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
    >
auto sinceHelper(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

class DnsResolver
{
public:

    struct HostInfo
    {
        std::string hostname;
        std::vector<std::string> addresses;
        bool error = false;
    };

    explicit DnsResolver(std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback);
    ~DnsResolver();
    DnsResolver(const DnsResolver &) = delete;
    DnsResolver &operator=(const DnsResolver &) = delete;

    void resolveDomains(const std::vector<std::string> &hostnames);
    void cancelAll();

private:
    // the maximum time for which there will be retries to make DNS queries for failed responses
    static constexpr int kMaxTimeoutMs = 10 * 1000;
    // time after which to repeat failed requests
    static constexpr int kRetryTimeoutMs = 500;

    std::function<void(std::map<std::string, HostInfo>)> resolveDomainsCallback_;
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    std::optional<boost::asio::deadline_timer> timer_;
    std::thread thread_;

    uint64_t curRequestId_ = 0;
    std::map<uint64_t, std::shared_ptr<wsnet::WSNetCancelableCallback>> activeRequests_;
    std::map<std::string, HostInfo> results_;

    std::chrono::time_point<std::chrono::steady_clock> startTime_;

    void onDnsResolved(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result);
    void onTimer(const boost::system::error_code& error);
};
