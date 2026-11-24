// The NetworkManager drop-in installed with the app must list exactly the tunnel interfaces the helper
// creates, or NetworkManager adopts the missing one and stalls on its connectivity check (see
// tunnel_interface_names.h). The drop-in is plain text in the installer tree, so this test reads it.

#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "../tunnel_interface_names.h"

namespace
{

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        fprintf(stderr, "FAILED line %d: %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

std::string trim(const std::string &s)
{
    const size_t b = s.find_first_not_of(" \t\r");
    const size_t e = s.find_last_not_of(" \t\r");
    return b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
}

// The interface names in the [keyfile] unmanaged-devices list. Entries are separated by ';' or ','.
std::set<std::string> unmanagedInterfaceNames(std::istream &in, int &listLines)
{
    std::set<std::string> names;
    std::string section;
    std::string line;
    listLines = 0;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = line;
            continue;
        }
        const std::string key = "unmanaged-devices+=";
        if (section != "[keyfile]" || line.compare(0, key.size(), key) != 0) {
            continue;
        }
        ++listLines;
        std::string entry;
        std::istringstream list(line.substr(key.size()));
        while (std::getline(list, entry, ';')) {
            std::string part;
            std::istringstream parts(entry);
            while (std::getline(parts, part, ',')) {
                part = trim(part);
                const std::string prefix = "interface-name:";
                VERIFY(part.compare(0, prefix.size(), prefix) == 0);
                names.insert(part.substr(prefix.size()));
            }
        }
    }
    return names;
}

// Returns false if this integration ships no drop-in, so the test reports a skip rather than a pass.
bool testDropInListsEveryTunnelInterface()
{
    std::ifstream file(WS_NM_DROP_IN_PATH);
    if (!file) {
        // An integration that ships no drop-in has nothing to keep in sync.
        printf("SKIP: no NetworkManager drop-in at %s\n", WS_NM_DROP_IN_PATH);
        return false;
    }
    int listLines = 0;
    const std::set<std::string> names = unmanagedInterfaceNames(file, listLines);
    // Appending (+=) rather than assigning keeps the distribution's own list, e.g. Ubuntu's.
    VERIFY(listLines == 1);
    const std::set<std::string> expected = {TunnelInterfaceNames::kWireGuard, TunnelInterfaceNames::kOpenVpn};
    VERIFY(names == expected);
    return true;
}

void testNamesFitAndAreDistinct()
{
    const std::set<std::string> names = {TunnelInterfaceNames::kWireGuard, TunnelInterfaceNames::kOpenVpn};
    VERIFY(names.size() == 2);
    for (const std::string &name : names) {
        VERIFY(!name.empty() && name.size() < 16);  // IFNAMSIZ includes the terminator.
    }
    // OpenVPN infers the device type from the "tun" prefix, and the client's tunnel checks
    // (route filter, default-route and DNS exclusions) match on "tun"/"utun".
    VERIFY(std::string(TunnelInterfaceNames::kOpenVpn).compare(0, 3, "tun") == 0);
}

}  // namespace

int main()
{
    const bool checkedDropIn = testDropInListsEveryTunnelInterface();
    testNamesFitAndAreDistinct();
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    if (!checkedDropIn) {
        return 77;  // SKIP_RETURN_CODE
    }
    printf("All tests passed\n");
    return 0;
}
