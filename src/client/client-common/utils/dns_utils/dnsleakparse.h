#pragma once

#include <functional>
#include <string>
#include <vector>

// The pure text→address parsers behind the OS resolver snapshot, single-sourced because the helper (nft
// DROP list) and the client (firewall allowlist) each used to hold a copy, so a parse bug had to be found
// twice. No Qt, so the root helper can compile it; no policy, which belongs at the call site.
//
// Entries are in the types::joinEndpoint packed form — canonical address plus ":port" when one was
// explicitly configured. Consumers split with types::splitEndpoint and must not assume a bare literal.
namespace DnsLeakParse {

// Sink for rejected tokens. No backend is baked in: the helper logs plain spdlog strings, while the
// client's default spdlog logger interpolates its payload into JSON, where a raw string is malformed.
using RejectHandler = std::function<void(const std::string &token)>;

// Validate, canonicalize and family-classify one token, then add it, deduping. The only supported way in:
// removeMatching compares addresses so entries must be canonical, and the helper renders the two lists
// into different nft expressions, so a misrouted entry makes a rule that fails the whole transaction.
void addAddress(const std::string &tok, std::vector<std::string> &v4, std::vector<std::string> &v6,
                const RejectHandler &onReject = nullptr);

// extraExemptHeaders exempts a link when one of its substrings appears in the header, for callers with
// weaker tunnel context than vpnIface (the client skips "(tun"/"(utun" since setVpnContext may not have
// run). The helper passes none: a blanket tun skip would stop it blocking another VPN's resolvers.
void parseResolvectl(const std::string &out, const std::string &vpnIface,
                     std::vector<std::string> &v4, std::vector<std::string> &v6,
                     const std::vector<std::string> &extraExemptHeaders = {},
                     const RejectHandler &onReject = nullptr);

void parseNmcli(const std::string &out, const std::string &vpnIface,
                std::vector<std::string> &v4, std::vector<std::string> &v6,
                const RejectHandler &onReject = nullptr);

// resolv.conf is flat rather than interface-scoped, so unlike the two above it carries no tunnel
// exemption — callers strip the tunnel's own resolvers with removeMatching afterwards. Text and file
// forms are separate so the parse is testable without a filesystem.
void parseResolvConfText(const std::string &text,
                         std::vector<std::string> &v4, std::vector<std::string> &v6,
                         const RejectHandler &onReject = nullptr);

// False when the path could not be opened, so a caller can tell that apart from "no nameserver lines".
bool parseResolvConfFile(const std::string &path,
                         std::vector<std::string> &v4, std::vector<std::string> &v6,
                         const RejectHandler &onReject = nullptr);

// Drop every entry whose ADDRESS matches one of allowedRaw, whatever port it carries. allowedRaw is the
// tunnel's own resolvers, which must never be blocked on any port. Both sides are canonicalized so a
// differently spelled address cannot slip the compare and leave the VPN's own resolver in the drop list.
void removeMatching(std::vector<std::string> &servers, const std::vector<std::string> &allowedRaw,
                    const RejectHandler &onReject = nullptr);

// All parsers APPEND and dedup against what is already there, so a caller can accumulate several sources
// into one pair of vectors. The helper relies on this: resolvectl, then nmcli into the same vectors when
// either family came back empty. Do not make a parser clear them.

} // namespace DnsLeakParse
