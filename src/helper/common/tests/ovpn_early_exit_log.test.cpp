// Pins the early-exit log extractor: only the fatal lines a user can act on survive,
// OpenVPN's timestamp prefix is dropped, and the result is capped.

#include <cstdio>
#include <string>

#include "../ovpn_early_exit_log.h"

namespace {

int g_failures = 0;

void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

void testKeepsFatalLineAndDropsNoise()
{
    const std::string log =
        "2026-10-08 12:00:00 OpenVPN 2.7.7\n"
        "2026-10-08 12:00:01 Windscribe: directive not permitted in config: up (config.ovpn:12)\n"
        "2026-10-08 12:00:01 Exiting due to fatal error\n";

    const std::string detail = OvpnEarlyExitLog::extract(log);
    VERIFY(detail == "Windscribe: directive not permitted in config: up (config.ovpn:12)");
}

void testKeepsOptionsError()
{
    const std::string log =
        "2026-10-08 12:00:01 us=123456 Options error: Unrecognized option or missing or extra parameter(s) in config.ovpn:3: remote (2.7.7)\n"
        "noise\n";

    const std::string detail = OvpnEarlyExitLog::extract(log);
    VERIFY(detail.find("Options error:") == 0);
    VERIFY(detail.find("us=123456") == std::string::npos);
}

void testCapsAt500()
{
    std::string line = "Windscribe: ";
    line.append(600, 'x');
    const std::string detail = OvpnEarlyExitLog::extract(line + "\n");
    VERIFY(detail.size() == 500);
    VERIFY(detail.find("Windscribe:") == 0);
}

void testEmptyWhenNothingActionable()
{
    VERIFY(OvpnEarlyExitLog::extract("2026-10-08 12:00:00 OpenVPN started\n").empty());
    VERIFY(OvpnEarlyExitLog::extract("").empty());
}

} // namespace

int main()
{
    testKeepsFatalLineAndDropsNoise();
    testKeepsOptionsError();
    testCapsAt500();
    testEmptyWhenNothingActionable();
    if (g_failures == 0) {
        printf("All ovpn early-exit log tests passed.\n");
        return 0;
    }
    printf("%d check(s) failed.\n", g_failures);
    return 1;
}
