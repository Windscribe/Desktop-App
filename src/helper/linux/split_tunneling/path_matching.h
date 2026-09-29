#pragma once

// Pure path-matching helpers for the split-tunneling process monitor.  Kept in a
// dependency-free header (std types only) so they can be unit-tested directly
// (../tests/pathmatching.test.cpp); the /proc readers stay in process_monitor.cpp.

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace path_matching {

inline std::vector<std::string> splitPathLower(const std::string &s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) {
                out.push_back(cur);
            }
            cur.clear();
        } else {
            cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

// Matches a Windows drive-mapped path token ("S:\common\Deadlock\game\bin\win64\deadlock.exe",
// optionally \\?\-prefixed) against a unix rule path.  Wine prefixes map Steam libraries to
// arbitrary drive letters, so the drive root is unknown; alignment is by path component:
// the rule's components-suffix must equal the token's components-head (case-insensitive,
// Windows path semantics).  At least two components must align, so generic single names
// ("bin") cannot produce a false positive.  A directory rule matches when the token path
// continues below the rule dir; a file rule only when the token is exactly that file.
// Known limitation: a prefix whose drive maps exactly to the rule dir itself (some Lutris
// setups) shares no path components with the rule and cannot match here.
inline bool windowsPathMatchesRule(const std::string &token, const std::string &rulePath, bool isDirectory)
{
    // Cheap prune before any copies: a Windows drive-mapped token must contain ':'.
    if (token.find(':') == std::string::npos) {
        return false;
    }
    std::string t = token;
    if (t.rfind("\\\\?\\", 0) == 0) {
        t = t.substr(4);
    }
    const size_t colon = t.find(':');
    if (colon == std::string::npos || colon > 1 || colon + 2 > t.size()) {
        return false;
    }
    if (t[colon + 1] != '\\' && t[colon + 1] != '/') {
        return false;
    }
    std::string rest = t.substr(colon + 2);
    std::replace(rest.begin(), rest.end(), '/', '\\'); // wine emits both separators
    std::vector<std::string> w = splitPathLower(rest, '\\');
    const std::vector<std::string> r = splitPathLower(rulePath, '/');
    if (w.size() < 2 || r.size() < 2) {
        return false;
    }

    // A root-mapped drive (wine's Z: -> /) renders the container's /run/host remap inside
    // the token ("\\?\Z:\run\host\usr\..."), while the rule names the host path below the
    // remap.  When the token's path head is run\host, the host-relative form drops it.
    const bool runHostRemapped = (w.size() > 4 && w[0] == "run" && w[1] == "host");

    const auto aligns = [&r, isDirectory](const std::vector<std::string> &wComponents) {
        const size_t kmax = std::min(wComponents.size(), r.size());
        for (size_t k = kmax; k >= 2; --k) {
            if (std::equal(wComponents.begin(), wComponents.begin() + k, r.end() - k)) {
                if (isDirectory || k == wComponents.size()) {
                    return true;
                }
            }
        }
        return false;
    };

    if (aligns(w)) {
        return true;
    }
    if (runHostRemapped) {
        w.erase(w.begin(), w.begin() + 2);
        return aligns(w);
    }
    return false;
}

// True when a process's exe path indicates a wine host binary (preloader, wine, wine64,
// wineserver), including pressure-vessel's /run/host rendering.  This gates the argv
// Windows-path matching the same way the script-matching gate requires the exe to be the
// shebang interpreter: a token that merely mentions a Windows path (in a file manager,
// editor, or the Steam client) must not classify a non-wine process.  "wine" is matched as
// a path-component prefix, so unrelated binaries like "rewinder" do not qualify.
inline bool exeLooksLikeWineHost(const std::string &exe)
{
    if (exe.find("/wine") != std::string::npos) {
        return true;
    }
    const size_t slash = exe.rfind('/');
    const std::string base = (slash == std::string::npos) ? exe : exe.substr(slash + 1);
    return base.rfind("wine", 0) == 0;
}

} // namespace path_matching
