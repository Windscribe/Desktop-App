#pragma once

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_set>

// OpenVPN directive filter for the privileged helper.
// This is the single source of truth for directive filtering.
//
// Three categories:
//   1. Allowed  — passes through to the generated config
//   2. Ignored  — silently stripped (directives the helper overrides anyway)
//   3. Blocked  — stripped with a warning log (security-relevant)
//
// References: OpenVPN 2.6 manual, Strix finding #1729

namespace OvpnDirectiveWhitelist {

inline const std::unordered_set<std::string>& allowedDirectives()
{
    static const std::unordered_set<std::string> directives = {
        // Connection & transport
        "client",
        "dev",
        "dev-type",
        "proto",
        "port",
        "remote",
        "remote-random",
        "remote-random-hostname",
        "resolv-retry",
        "nobind",
        "float",
        "connect-retry",
        "connect-retry-max",
        "connect-timeout",
        "server-poll-timeout",
        "explicit-exit-notify",
        "local",
        "proto-force",
        "socket-flags",
        "compat-mode",

        // Crypto & TLS
        "cipher",
        "data-ciphers",
        "data-ciphers-fallback",
        "ncp-ciphers",
        "auth",
        "key-direction",
        "remote-cert-tls",
        "ns-cert-type",
        "verify-x509-name",
        "tls-version-min",
        "tls-version-max",
        "tls-cipher",
        "tls-ciphersuites",
        "tls-groups",
        "tls-timeout",
        "tls-client",
        "single-session",
        "auth-nocache",
        "tran-window",
        "reneg-sec",
        "hand-window",

        // Authentication
        // auth-user-pass is allowed only in its bare form (see bareOnlyDirectives()
        // below). The file-path form (auth-user-pass <file>) is stripped to prevent
        // attacker-controlled configs from reading arbitrary files. The bare form
        // is required so OpenVPN queries the management interface for credentials;
        // without it, management-query-passwords has nothing to drive.
        // Inline block form (<auth-user-pass>...</auth-user-pass>) is allowed via
        // allowedInlineBlockTags().
        "auth-user-pass",
        "auth-retry",
        "auth-token",
        "auth-token-user",
        "static-challenge",

        // Routing, DNS & network
        "route",
        "redirect-gateway",
        "route-nopull",
        "pull",
        "pull-filter",
        "dhcp-option",
        "dns",
        "allow-pull-fqdn",
        "topology",
        "ifconfig",
        "allow-recursive-routing",
        "client-nat",

        // Tunneling, MTU & performance
        "persist-key",
        "persist-tun",
        "persist-local-ip",
        "persist-remote-ip",
        "tun-mtu",
        "fragment",
        "mssfix",
        "mtu-disc",
        "link-mtu",
        "sndbuf",
        "rcvbuf",
        "keepalive",
        "ping",
        "ping-restart",
        "ping-exit",
        "fast-io",
        "mlock",
        "inactive",

        // Compression
        "comp-lzo",
        "compress",
        "comp-noadapt",
        "allow-compression",

        // Logging (safe subset)
        "mute",
        "mute-replay-warnings",
        "ignore-unknown-option",
        "echo",
        "machine-readable-output",

        // Client options
        "push-peer-info",

        // Anti-censorship (Windscribe custom, patched OpenVPN)
        "udp-stuffing",
        "tcp-split-reset",
        "i1",
        "i2",
        "i3",
        "i4",
        "i5"
    };
    return directives;
}

// Directives that the helper overrides with its own values.
// These are silently stripped without a warning log since they have no effect
// (the helper appends its own values after filtering).
inline const std::unordered_set<std::string>& ignoredDirectives()
{
    static const std::unordered_set<std::string> directives = {
        "verb",
        "script-security",
        "up",
        "down",
        "down-pre",
        "management",
        "management-query-passwords",
        "management-hold",
        "http-proxy",
        "socks-proxy",
        "dev-node",
        "disable-dco",
    };
    return directives;
}

// Directives that are allowed only in their bare form (no arguments).
// Lines like "<directive> <anything>" are stripped even when the directive is
// in allowedDirectives(). Inline comments (# or ;) after the directive are not
// treated as arguments.
inline const std::unordered_set<std::string>& bareOnlyDirectives()
{
    static const std::unordered_set<std::string> directives = {
        "auth-user-pass",
    };
    return directives;
}

// Extract the directive name from a config line.
// Strips optional leading "--" prefix and returns the first whitespace-delimited token,
// lowercased for case-insensitive matching.
inline std::string extractDirectiveName(const std::string &line)
{
    size_t start = 0;

    // Skip leading whitespace
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }

    // Skip optional "--" prefix
    if (start + 1 < line.size() && line[start] == '-' && line[start + 1] == '-') {
        start += 2;
    }

    // Find end of directive name
    size_t end = start;
    while (end < line.size() && line[end] != ' ' && line[end] != '\t' && line[end] != '\n' && line[end] != '\r') {
        ++end;
    }

    std::string name = line.substr(start, end - start);

    // Lowercase
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return name;
}

// Returns true if the line has any non-comment, non-whitespace token after the
// directive name. Inline comments (# or ;) are not treated as arguments.
inline bool hasNonCommentArgument(const std::string &line)
{
    size_t pos = 0;
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }
    if (pos + 1 < line.size() && line[pos] == '-' && line[pos + 1] == '-') {
        pos += 2;
    }
    while (pos < line.size() && line[pos] != ' ' && line[pos] != '\t' &&
           line[pos] != '\n' && line[pos] != '\r') {
        ++pos;
    }
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
        ++pos;
    }
    if (pos >= line.size() || line[pos] == '#' || line[pos] == ';' ||
        line[pos] == '\n' || line[pos] == '\r') {
        return false;
    }
    return true;
}

// Known inline block tag names whose content (certificate data, key material,
// etc.) should be passed through without directive filtering.
inline const std::unordered_set<std::string>& allowedInlineBlockTags()
{
    static const std::unordered_set<std::string> tags = {
        "ca", "cert", "key", "tls-auth", "tls-crypt", "tls-crypt-v2",
        "pkcs12", "extra-certs", "auth-user-pass", "secret",
        "dh", "http-proxy-user-pass",
    };
    return tags;
}

// Extract the tag name from an inline block line like "<ca>" or "</tls-auth>".
// Returns empty string if the line is not a valid inline block tag.
inline std::string extractInlineBlockTag(const std::string &line, size_t start, bool &isClosing)
{
    isClosing = false;
    bool closing = false;
    // Must start with '<'
    if (start >= line.size() || line[start] != '<') {
        return "";
    }

    size_t pos = start + 1;

    // Check for closing tag
    if (pos < line.size() && line[pos] == '/') {
        closing = true;
        ++pos;
    }

    // Extract tag name until '>'
    size_t nameStart = pos;
    while (pos < line.size() && line[pos] != '>' && line[pos] != ' ' && line[pos] != '\t') {
        ++pos;
    }

    if (pos >= line.size() || line[pos] != '>') {
        return "";  // No closing '>', not a valid tag
    }

    std::string tag = line.substr(nameStart, pos - nameStart);

    // Reject anything other than whitespace after '>'. OpenVPN treats inline
    // tags as line-anchored; if we accepted "<ca> trailing-data" as an opener
    // while OpenVPN did not, blocked directives placed inside the apparent
    // block could survive filtering.
    ++pos;
    while (pos < line.size()) {
        if (line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\r' && line[pos] != '\n') {
            return "";
        }
        ++pos;
    }

    // Lowercase
    std::transform(tag.begin(), tag.end(), tag.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    isClosing = closing;
    return tag;
}

// Check whether all inline block tags in the config are properly balanced.
// Returns true if every opening tag has a matching closing tag.
// Callers should use this as a pre-scan before filtering with isAllowed().
// If tags are unbalanced, callers should disable inline block pass-through
// by passing allowInlineBlocks=false to isAllowed().
inline bool hasBalancedInlineBlocks(const std::string &config)
{
    std::istringstream stream(config);
    std::string line;
    bool inside = false;
    std::string currentTag;

    while (std::getline(stream, line)) {
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
            ++start;
        }
        if (start >= line.size() || line[start] != '<') {
            continue;
        }

        bool isClosing = false;
        std::string tag = extractInlineBlockTag(line, start, isClosing);
        if (tag.empty() || allowedInlineBlockTags().count(tag) == 0) {
            continue;
        }

        if (!isClosing) {
            if (inside) {
                return false;  // Nested open tags
            }
            inside = true;
            currentTag = tag;
        } else {
            if (!inside || tag != currentTag) {
                return false;  // Close without open, or mismatched
            }
            inside = false;
            currentTag.clear();
        }
    }

    return !inside;  // False if still inside an unclosed block
}

// Returns true if the directive on this line is allowed.
// Empty lines, comments (#, ;), and inline block tags (<...>) are always allowed.
// The insideInlineBlock parameter tracks whether we are inside a known inline
// block (e.g. between <ca> and </ca>). Content inside known inline blocks is
// allowed (certificate data, key material, etc.). Only recognized inline block
// tags enter pass-through mode — unknown tags do not.
// If allowInlineBlocks is false (e.g. because tags are unbalanced), inline
// block content is NOT passed through — only the tags themselves are allowed.
// Callers must initialize insideInlineBlock to false and pass the same
// variable for each line.
inline bool isAllowed(const std::string &line, bool &insideInlineBlock, bool allowInlineBlocks = true)
{
    size_t start = 0;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }

    // Empty or comment lines are always allowed
    if (start >= line.size() || line[start] == '#' || line[start] == ';') {
        return true;
    }

    // Handle inline block tags (e.g. <ca>, </ca>, <tls-auth>, </tls-auth>)
    if (line[start] == '<') {
        bool isClosing = false;
        std::string tag = extractInlineBlockTag(line, start, isClosing);

        if (!tag.empty() && allowedInlineBlockTags().count(tag) > 0) {
            if (allowInlineBlocks) {
                insideInlineBlock = isClosing ? false : true;
            }
            return true;
        }

        // Unknown tag — don't enter pass-through mode, treat as regular line
        // (will fall through to directive check below, and likely be blocked)
    }

    // Everything inside a known inline block is allowed (cert data, key material, etc.)
    if (insideInlineBlock) {
        return true;
    }

    std::string name = extractDirectiveName(line);
    if (name.empty()) {
        return true;
    }

    if (allowedDirectives().count(name) == 0) {
        return false;
    }
    if (bareOnlyDirectives().count(name) > 0 && hasNonCommentArgument(line)) {
        return false;
    }
    return true;
}

// Filter an entire OpenVPN config string, returning only allowed lines.
// This is the main entry point for helper code. It performs:
//   1. Balance check on inline block tags
//   2. Line-by-line whitelist filtering with inline block context tracking
// Blocked directives are reported via onBlocked; ignored directives (those the
// helper overrides with its own value) are reported via onIgnored.
// Lines are newline-terminated in the output.
inline std::string filterConfig(const std::string &config,
    std::function<void(const std::string &)> onBlocked = nullptr,
    std::function<void(const std::string &)> onIgnored = nullptr)
{
    const bool balancedBlocks = hasBalancedInlineBlocks(config);

    std::istringstream stream(config);
    std::string line;
    std::string result;
    bool insideInlineBlock = false;

    while (std::getline(stream, line)) {
        // Trim whitespace
        size_t first = line.find_first_not_of(" \n\r\t");
        size_t last = line.find_last_not_of(" \n\r\t");
        if (first == std::string::npos) {
            line.clear();
        } else {
            line = line.substr(first, last - first + 1);
        }

        if (!isAllowed(line, insideInlineBlock, balancedBlocks)) {
            std::string name = extractDirectiveName(line);
            if (!name.empty()) {
                if (ignoredDirectives().count(name) > 0) {
                    if (onIgnored) {
                        onIgnored(name);
                    }
                } else if (onBlocked) {
                    onBlocked(name);
                }
            }
            continue;
        }

        result += line;
        result += '\n';
    }

    return result;
}

} // namespace OvpnDirectiveWhitelist
