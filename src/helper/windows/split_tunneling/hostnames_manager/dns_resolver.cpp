#include "dns_resolver.h"
#include <spdlog/spdlog.h>

using namespace wsnet;

DnsResolver::DnsResolver(std::function<void (std::map<std::string, HostInfo>)> resolveDomainsCallback) :
    resolveDomainsCallback_(resolveDomainsCallback),
    work_(boost::asio::make_work_guard(io_context_))
{
    thread_ = std::thread([this](){ io_context_.run(); });
}

DnsResolver::~DnsResolver()
{
    io_context_.stop();
    thread_.join();
    // isWSNetInitialized_ is only written on the io_context thread, which has been joined above.
    if (isWSNetInitialized_)
        WSNet::cleanup();
}

bool DnsResolver::ensureWSNetInitialized()
{
    // Called only on the io_context thread. WSNet is initialized lazily on the first hostname lookup
    // so the helper doesn't spin up wsnet (threads, c-ares, etc.) unless split tunneling actually needs it.
    if (isWSNetInitialized_)
        return true;
    if (isWSNetInitFailed_)
        return false;

    // will not log the output from the library, too much unnecessary information
    if (!WSNet::initialize("", "", "", "", "", "", false, "en", "", nullptr, false, "")) {
        spdlog::error("WSNet::initialize failed");
        // On failure WSNet::initialize leaves a partially initialized instance behind. It can't be safely
        // released (e.g. DnsResolver_cares joins a thread it never started) and can't be re-initialized
        // (WSNet::initialize asserts there is no existing instance), so leave it and don't retry.
        isWSNetInitFailed_ = true;
        return false;
    }

    isWSNetInitialized_ = true;
    return true;
}

void DnsResolver::onDnsResolved(uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result)
{
    boost::asio::post(io_context_,[this, requestId, hostname, result] {
        auto request = activeRequests_.find(requestId);
        if (request == activeRequests_.end())
            return;

        HostInfo hi;
        hi.hostname = hostname;
        hi.addresses = result->ips();
        hi.error = !result->error()->isSuccess();
        results_[hostname] = hi;
        activeRequests_.erase(request);
        if (activeRequests_.empty()) {
            // check if there were failed requests
            bool bWasFailedRequests = false;
            for (const auto &it: results_) {
                if (it.second.error) {
                    bWasFailedRequests = true;
                    break;
                }
            }
            // repeat failed requests if the timeout allows, after 500 ms
            if (bWasFailedRequests && sinceHelper(startTime_).count() < kMaxTimeoutMs) {
                timer_.emplace(io_context_, std::chrono::milliseconds(kRetryTimeoutMs));
                timer_->async_wait(std::bind(&DnsResolver::onTimer, this, std::placeholders::_1));
            } else {
                resolveDomainsCallback_(results_);
            }
        }
    });
}

void DnsResolver::onTimer(const boost::system::error_code &error)
{
    // if the timer has not been cancelled
    if (error.value() == 0) {
        // repeat failed requests if the timeout allows
        if (sinceHelper(startTime_).count() >= kMaxTimeoutMs) {
            resolveDomainsCallback_(results_);
        } else {
            for (const auto &it: results_) {
                if (it.second.error) {
                    using namespace std::placeholders;
                    auto request = WSNet::instance()->dnsResolver()->lookup(it.first, curRequestId_, IpFamily::kBoth, std::bind(&DnsResolver::onDnsResolved, this, _1, _2, _3));
                    activeRequests_.insert(std::make_pair(curRequestId_, request));
                }
            }
        }
    }
}

void DnsResolver::resolveDomains(const std::vector<std::string> &hostnames)
{
    timer_.reset();
    cancelAll();

    boost::asio::post(io_context_, [this, hostnames] {
        if (hostnames.empty())
            return;

        if (!ensureWSNetInitialized()) {
            std::map<std::string, HostInfo> failed;
            for (const auto &hostname : hostnames) {
                HostInfo hi;
                hi.hostname = hostname;
                hi.error = true;
                failed[hostname] = hi;
            }
            resolveDomainsCallback_(failed);
            return;
        }

        startTime_ = std::chrono::steady_clock::now();
        for (const auto &hostname : hostnames) {
            using namespace std::placeholders;
            auto request = WSNet::instance()->dnsResolver()->lookup(hostname, curRequestId_, IpFamily::kBoth, std::bind(&DnsResolver::onDnsResolved, this, _1, _2, _3));
            activeRequests_.insert(std::make_pair(curRequestId_, request));
            curRequestId_++;
        }
    });
}

void DnsResolver::cancelAll()
{
    boost::asio::post(io_context_, [this] {
        for (auto &request : activeRequests_)
            request.second->cancel();
        activeRequests_.clear();
        results_.clear();
    });
}
