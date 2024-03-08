#pragma once

#include <string>
#include <assert.h>
#include "utils/utils.h"
#include <fmt/format.h>

namespace wsnet {

class FailoverData {

public:
    explicit FailoverData(const std::string &domain) : domain_(domain) {}
    explicit FailoverData(const std::string &domain, const std::string &sniDomain) : domain_(domain), sniDomain_(sniDomain) {}
    explicit FailoverData(const std::string &domain, const std::string &echConfig, int ttl) :
        domain_(domain), echConfig_(echConfig), ttl_(ttl)
    {
        startTime_ = std::chrono::steady_clock::now();
    }

    std::string domain() const { return domain_; }
    std::string echConfig() const { return echConfig_; }
    std::string sniDomain() const { return sniDomain_; }
    int ttl() const { return ttl_; }
    bool isExpired() const
    {
        assert(!echConfig_.empty());
        return utils::since(startTime_).count() > (ttl_ * 1000);
    }

private:
    std::string   domain_;
    std::string   sniDomain_;     // empty if no SNI spoofing / domain fronting
    std::string   echConfig_;     // empty if it does not support ECH
    int ttl_ = 0;                 // TTL in seconds
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace wsnet

// formatter for fmt lib
// basically for debug purpose
template<>
struct fmt::formatter<wsnet::FailoverData>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(wsnet::FailoverData const& fd, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "domain: {0}, ech: {1}, sniDomain: {2}, ttl: {3}", fd.domain(), fd.echConfig(), fd.sniDomain(), fd.ttl());
    }
};
