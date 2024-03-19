#include "dns_resolver.h"
#include "../../logger.h"

using namespace wsnet;

DnsResolver::DnsResolver(std::function<void (std::map<std::string, HostInfo>)> resolveDomainsCallback) :
    resolveDomainsCallback_(resolveDomainsCallback),
    work_(boost::asio::make_work_guard(io_service_))
{
    WSNet::setLogger([](const std::string &logStr) {
        Logger::instance().out("[wsnet] %s", logStr.c_str());
    }, false);


    if (!WSNet::initialize("", "", false, "")) {
        Logger::instance().out("WSNet::initialize failed");
    }

    thread_ = std::thread([this](){ io_service_.run(); });
}

DnsResolver::~DnsResolver()
{
    io_service_.stop();
    thread_.join();
    WSNet::cleanup();
}

void DnsResolver::onDnsResolved(uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result)
{
    boost::asio::post(io_service_,[this, requestId, hostname, result] {
        auto request = activeRequests_.find(requestId);
        if (request == activeRequests_.end())
            return;

        HostInfo hi;
        hi.hostname = hostname;
        hi.addresses = result->ips();
        hi.error = result->isError();
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
                timer_.emplace(boost::asio::deadline_timer(io_service_, boost::posix_time::millisec(kRetryTimeoutMs)));
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
                    auto request = WSNet::instance()->dnsResolver()->lookup(it.first, curRequestId_, std::bind(&DnsResolver::onDnsResolved, this, _1, _2, _3));
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

    boost::asio::post(io_service_, [this, hostnames] {
        startTime_ = std::chrono::steady_clock::now();
        for (const auto &hostname : hostnames) {
            using namespace std::placeholders;
            auto request = WSNet::instance()->dnsResolver()->lookup(hostname, curRequestId_, std::bind(&DnsResolver::onDnsResolved, this, _1, _2, _3));
            activeRequests_.insert(std::make_pair(curRequestId_, request));
            curRequestId_++;
        }
    });
}

void DnsResolver::cancelAll()
{
    boost::asio::post(io_service_, [this] {
        for (auto &request : activeRequests_)
            request.second->cancel();
        activeRequests_.clear();
        results_.clear();
    });
}
