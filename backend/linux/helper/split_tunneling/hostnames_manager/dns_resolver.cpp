#include "dns_resolver.h"
#include "../../logger.h"

using namespace wsnet;

DnsResolver::DnsResolver(std::function<void (std::map<std::string, HostInfo>)> resolveDomainsCallback) :
    resolveDomainsCallback_(resolveDomainsCallback),
    taskQueue_(1)
{
    if (!WSNet::initialize("", "", false, "")) {
        Logger::instance().out("WSNet::initialize failed");
    }
}

DnsResolver::~DnsResolver()
{
    taskQueue_.wait();
    WSNet::cleanup();
}

void DnsResolver::onDnsResolved(uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result)
{
    taskQueue_.detach_task([this, requestId, hostname, result] {
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
            resolveDomainsCallback_(results_);
        }
    });
}

void DnsResolver::resolveDomains(const std::vector<std::string> &hostnames)
{
    cancelAll();

    // Execute in a thread pool to avoid synchronisation issues
    taskQueue_.detach_task([this, hostnames] {
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
    taskQueue_.detach_task([this] {
        for (auto &request : activeRequests_)
            request.second->cancel();
        activeRequests_.clear();
        results_.clear();
    });
}
