#include "dnsleakparse.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "types/ipaddress.h"

namespace DnsLeakParse {

namespace {

void reject(const RejectHandler &onReject, const std::string &tok)
{
    if (onReject) {
        onReject(tok);
    }
}

// The local stub resolver — 127.0.0.53 under systemd-resolved, or a loopback/unspecified entry from a
// hand-written resolv.conf. Never blocked and never needs allowlisting, so it must not enter the
// snapshot: counting it as a discovered resolver suppressed both callers' "no resolvers found" warnings
// while producing no rule at all, which reads as success on a host with no protection.
bool isLocalStub(const types::IpAddress &address)
{
    return address.isLoopbackOrUnspecified();
}

// Internal on purpose: appending raw bypasses the validation, canonicalization and family classification
// that addAddress enforces. Callers outside get addAddress only.
void pushUnique(std::vector<std::string> &v, const std::string &s)
{
    if (std::find(v.begin(), v.end(), s) == v.end()) {
        v.push_back(s);
    }
}

// Tokenize a whitespace-separated address list and classify each token. Shared by the resolvectl and
// nmcli parsers, which both reduce to "addresses after a colon".
void addTokens(const std::string &list, std::vector<std::string> &v4, std::vector<std::string> &v6,
               const RejectHandler &onReject)
{
    std::istringstream ts(list);
    std::string tok;
    while (ts >> tok) {
        addAddress(tok, v4, v6, onReject);
    }
}

} // namespace

// resolvectl prints the extended form — address, optional port (bracketed when present), optional numeric
// zone id, optional DoT server name: "192.0.2.1:5353", "[fe80::1]:5353%3#dns.example.com", "fe80::1%3".
// Strip the suffixes that are not part of the endpoint, then let types::splitEndpoint do the rest.

// The port is kept, not discarded: it is the only port that resolver answers on, so a caller emitting
// port-specific rules must use it instead of assuming the well-known ones.
void addAddress(const std::string &token, std::vector<std::string> &v4, std::vector<std::string> &v6,
                const RejectHandler &onReject)
{
    std::string endpoint = token;
    const auto hash = endpoint.find('#');
    if (hash != std::string::npos) {
        endpoint = endpoint.substr(0, hash);
    }
    // The zone id trails the port in the bracketed form and the address in the bare one, and nothing
    // valid follows it once the server name is gone, so cutting at the first '%' covers both.
    const auto pct = endpoint.find('%');
    if (pct != std::string::npos) {
        endpoint = endpoint.substr(0, pct);
    }
    if (endpoint.empty()) {
        return;
    }

    std::string host;
    uint16_t port = 0;
    if (!types::splitEndpoint(endpoint, host, port)) {
        reject(onReject, token);
        return;
    }
    const types::IpAddress addr(host);
    if (!addr.isValid()) {
        reject(onReject, token);
        return;
    }
    if (isLocalStub(addr)) {
        return;
    }
    const std::string canonical = addr.toString();
    // Store the canonical inet_ntop form, not the raw token, so removeMatching's address compare cannot
    // miss an IPv6 server printed non-canonically and leave the tunnel's own resolver in the drop list.
    pushUnique(addr.isV6() ? v6 : v4, types::joinEndpoint(canonical, port));
}

// Parse `resolvectl dns` output: "Link N (ifc): addr addr ...", skipping the line whose header names the
// tunnel so we never blacklist our own VPN DNS.
//
// The list is NOT guaranteed to fit one line. resolvectl wraps at columns() — 80 whenever stdout is not a
// tty, always here — continuing on lines indented by strlen("Global: ") with no header. A continuation
// inherits the preceding header's exemption and is consumed whole; splitting it on the first colon
// truncated a leading IPv6 into an invalid token and dropped colon-less IPv4 lines outright, both losing
// live resolvers while enable() still reported success.
void parseResolvectl(const std::string &out, const std::string &vpnIface,
                     std::vector<std::string> &v4, std::vector<std::string> &v6,
                     const std::vector<std::string> &extraExemptHeaders,
                     const RejectHandler &onReject)
{
    const auto isExempt = [&](const std::string &head) {
        if (!vpnIface.empty() && head.find("(" + vpnIface + ")") != std::string::npos) {
            return true;
        }
        for (const auto &token : extraExemptHeaders) {
            if (head.find(token) != std::string::npos) {
                return true;
            }
        }
        return false;
    };

    std::istringstream s(out);
    std::string line;
    bool skipLink = false;
    while (std::getline(s, line)) {
        // resolvectl indents continuations with spaces, never tabs (printf "%*s").
        if (!line.empty() && line[0] == ' ') {
            if (!skipLink) {
                addTokens(line, v4, v6, onReject);
            }
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            // Not a link header, so no exemption can be attributed to it — clear the previous one
            // rather than letting it carry onto following continuations.
            skipLink = false;
            continue;
        }
        skipLink = isExempt(line.substr(0, colon));
        if (skipLink) {
            continue;
        }
        addTokens(line.substr(colon + 1), v4, v6, onReject);
    }
}

// Parse `nmcli dev show` output: rows "IP4.DNS[n]: <addr>" / "IP6.DNS[n]: <addr>". Classification is
// by actual family (addAddress), so a value lands in the right list regardless of the row label. The
// output is grouped per device by a leading "GENERAL.DEVICE: <name>" row; track the current device
// and skip the tunnel's own block so we never blacklist our own VPN DNS — the same tunnel-link
// exemption parseResolvectl applies via its header filter.
void parseNmcli(const std::string &out, const std::string &vpnIface,
                std::vector<std::string> &v4, std::vector<std::string> &v6,
                const RejectHandler &onReject)
{
    std::istringstream s(out);
    std::string line;
    std::string curDev;
    while (std::getline(s, line)) {
        if (line.rfind("GENERAL.DEVICE", 0) == 0) {
            const auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::istringstream ds(line.substr(colon + 1));
                curDev.clear();
                ds >> curDev;
            }
            continue;
        }
        // nmcli prints field names at column 0 ("IP4.DNS[1]:    1.2.3.4"), so anchor the match instead
        // of scanning the whole line — a value can never be mistaken for a field name.
        if (line.rfind("IP4.DNS", 0) != 0 && line.rfind("IP6.DNS", 0) != 0) {
            continue;
        }
        if (!vpnIface.empty() && curDev == vpnIface) {
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        addTokens(line.substr(colon + 1), v4, v6, onReject);
    }
}

// Parse "nameserver <addr>" lines, reading only the first argument as glibc does. The format has no
// trailing-comment syntax, so "nameserver 192.0.2.1 # was 8.8.8.8" would otherwise contribute 8.8.8.8 —
// harmless over-blocking in the helper, but an unconfigured address allowlisted through the client.
void parseResolvConfText(const std::string &text, std::vector<std::string> &v4, std::vector<std::string> &v6,
                         const RejectHandler &onReject)
{
    std::istringstream s(text);
    std::string line;
    while (std::getline(s, line)) {
        // glibc and musl both anchor the keyword at column 0, so an indented line is a disabled entry —
        // indenting is a common way to comment one out — and must not be read back as a live resolver.
        if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
            continue;
        }
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key) || key != "nameserver") {
            continue;
        }
        std::string addr;
        if (ls >> addr) {
            addAddress(addr, v4, v6, onReject);
        }
    }
}

// False when the file could not be opened. Callers must report that distinctly: unreadable and "no
// nameserver lines" otherwise both read as "no resolvers configured", which sends whoever reads the log
// to the resolver config instead of to file permissions.
bool parseResolvConfFile(const std::string &path, std::vector<std::string> &v4, std::vector<std::string> &v6,
                         const RejectHandler &onReject)
{
    std::ifstream f(path);
    if (!f) {
        return false;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    parseResolvConfText(buf.str(), v4, v6, onReject);
    return true;
}

void removeMatching(std::vector<std::string> &servers, const std::vector<std::string> &allowedRaw,
                    const RejectHandler &onReject)
{
    std::vector<std::string> allowed;
    allowed.reserve(allowedRaw.size());
    for (const auto &ip : allowedRaw) {
        const types::IpAddress parsed(ip);
        if (parsed.isValid()) {
            allowed.push_back(parsed.toString());
        } else {
            reject(onReject, ip);
        }
    }
    servers.erase(std::remove_if(servers.begin(), servers.end(),
                                 [&allowed](const std::string &entry) {
                                     std::string addr;
                                     uint16_t port = 0;
                                     if (!types::splitEndpoint(entry, addr, port)) {
                                         return false;
                                     }
                                     return std::find(allowed.begin(), allowed.end(), addr) != allowed.end();
                                 }),
                  servers.end());
}

} // namespace DnsLeakParse
