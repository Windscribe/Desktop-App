#include "emergencyconnect.h"
#include <cmrc/cmrc.hpp>
#include <spdlog/spdlog.h>
#include "failover/failovercontainer.h"
#include "emergencyconnectendpoint.h"
#include "utils/utils.h"

#ifndef FAILOVER_CONTAINER_PUBLIC
    #include "privatesettings.h"
#endif

CMRC_DECLARE(wsnet);

namespace wsnet {

EmergencyConnect::EmergencyConnect(boost::asio::io_context &io_context, IFailoverContainer *failoverContainer, WSNetDnsResolver *dnsResolver) :
    io_context_(io_context),
    failoverContainer_(failoverContainer),
    dnsResolver_(dnsResolver)
{
}

EmergencyConnect::~EmergencyConnect()
{
    for (auto &it : dnsRequests_)
        it.second.first->cancel();
}

std::string EmergencyConnect::ovpnConfig() const
{
    auto fs = cmrc::wsnet::get_filesystem();
    assert(fs.is_file("resources/emergency.ovpn"));
    auto ovpn = fs.open("resources/emergency.ovpn");
    return std::string(ovpn.begin(), ovpn.end());
}

std::string EmergencyConnect::username() const
{
#ifndef FAILOVER_CONTAINER_PUBLIC
    return PrivateSettings::instance().emergencyUsername();
#else
    return std::string();
#endif
}

std::string EmergencyConnect::password() const
{
#ifndef FAILOVER_CONTAINER_PUBLIC
    return PrivateSettings::instance().emergencyPassword();
#else
    return std::string();
#endif
}

std::shared_ptr<WSNetCancelableCallback> EmergencyConnect::getIpEndpoints(WSNetEmergencyConnectCallback callback)
{
#ifndef FAILOVER_CONTAINER_PUBLIC
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetEmergencyConnectCallback>>(callback);
    boost::asio::post(io_context_, [this, cancelableCallback] {
        auto failover = failoverContainer_->failoverById(FAILOVER_OLD_RANDOM_DOMAIN_GENERATION);
        assert(failover);
        std::vector<FailoverData> data;
        bool isSuccess = failover->getData(false, data, nullptr);
        assert(isSuccess);
        assert(data.size() == 1);
        std::string hashedDomain = "econnect." + data[0].domain();

        using namespace std::placeholders;
        auto dnsRequest = dnsResolver_->lookup(hashedDomain, curRequestId_, std::bind(&EmergencyConnect::onDnsResolved, this, _1, _2, _3));
        dnsRequests_[curRequestId_] = std::make_pair(dnsRequest, cancelableCallback);
        curRequestId_++;
    });

    return cancelableCallback;
#else
    return nullptr;
#endif
}

void EmergencyConnect::onDnsResolved(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<WSNetDnsRequestResult> result)
{
#ifndef FAILOVER_CONTAINER_PUBLIC

    boost::asio::post(io_context_, [this, requestId, hostname, result] {
        auto it = dnsRequests_.find(requestId);
        if (it == dnsRequests_.end())
            return;

        std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> endpoints;
        if (!result->isError()) {
            std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> list;
            auto ips = result->ips();
            ips = utils::randomizeList(ips);
            for (const auto &ip : ips) {
                endpoints.push_back(std::make_shared<EmergencyConnectEndpoint>(ip, 443, Protocol::kUdp));
                endpoints.push_back(std::make_shared<EmergencyConnectEndpoint>(ip, 443, Protocol::kTcp));
            }
        } else {
            spdlog::warn("EmergencyConnect::onDnsResolved failed");
        }

        std::vector<std::shared_ptr<WSNetEmergencyConnectEndpoint>> endpointsHardcoded;
        endpointsHardcoded.push_back(std::make_shared<EmergencyConnectEndpoint>(PrivateSettings::instance().emergencyIP1(), 1194, Protocol::kUdp));
        endpointsHardcoded.push_back(std::make_shared<EmergencyConnectEndpoint>(PrivateSettings::instance().emergencyIP2(), 1194, Protocol::kUdp));
        endpointsHardcoded = utils::randomizeList(endpointsHardcoded);

        endpoints.insert(endpoints.end(), endpointsHardcoded.begin(), endpointsHardcoded.end());

        it->second.second->call(endpoints);
        dnsRequests_.erase(it);
    });
#endif
}


} // namespace wsnet
