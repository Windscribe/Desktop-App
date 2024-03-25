#pragma once

#include <string>
#include <optional>
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
    std::optional<int> ttl() const { return ttl_; }
    bool isExpired() const
    {
        if (ttl_.has_value())
            return utils::since(startTime_).count() > (ttl_.value() * 1000);
        else
            return false;
    }

    // needed for std::set<FailoverData>
    bool operator<(const FailoverData& rhs) const
    {
        // ignore ttl_ and startTime_
        std::string uniqId = domain_ + sniDomain_ + echConfig_;
        std::string uniqId_rhs = rhs.domain_ + rhs.sniDomain_ + rhs.echConfig_;
        return uniqId < uniqId_rhs;
    }

private:
    std::string   domain_;
    std::string   sniDomain_;     // empty if no SNI spoofing / domain fronting
    std::string   echConfig_;     // empty if it does not support ECH
    std::optional<int> ttl_;      // TTL in seconds
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
        if (fd.ttl().has_value())
            return fmt::format_to(ctx.out(), "domain: {0}, ech: {1}, sniDomain: {2}, ttl: {3}", fd.domain(), fd.echConfig(), fd.sniDomain(), fd.ttl().value());
        else
            return fmt::format_to(ctx.out(), "domain: {0}, ech: {1}, sniDomain: {2}", fd.domain(), fd.echConfig(), fd.sniDomain());
    }
};
