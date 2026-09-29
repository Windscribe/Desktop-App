// Regression tests for the split-tunneling process monitor's wine/Proton path matching
// (path_matching.h).  A Proton game's /proc/<pid>/exe is the wine preloader inside the
// Proton install, never the game binary, so directory rules can never match the exe; the
// fallback signals are the process working directory and argv[0]'s Windows drive-mapped
// path (live-observed form: "S:\common\Deadlock\game\bin\win64\deadlock.exe" against rule
// /mnt/big/SteamLibrary/steamapps/common/Deadlock).  These cases pin the matching
// semantics and the false-positive gates.  No external test framework; returns the number
// of failed checks (0 on success).

#include <cstdio>
#include <string>

#include "../split_tunneling/path_matching.h"

using namespace path_matching;

namespace
{

int g_failures = 0;

bool check(bool ok, const char *expr, int line)
{
    if (!ok) {
        ++g_failures;
        printf("FAIL (line %d): %s\n", line, expr);
    }
    return ok;
}

#define VERIFY(expr) check(!!(expr), #expr, __LINE__)

} // namespace

int main()
{
    const std::string deadlockDir = "/mnt/big/SteamLibrary/steamapps/common/Deadlock";
    const std::string dotaDir = "/mnt/big/SteamLibrary/steamapps/common/dota 2 beta";
    const std::string deadlockExeRule = deadlockDir + "/game/bin/win64/deadlock.exe";

    // The exact argv[0] observed on a live Deadlock process.
    VERIFY(windowsPathMatchesRule("S:\\common\\Deadlock\\game\\bin\\win64\\deadlock.exe", deadlockDir, true));
    // Spaces inside argv[0] are legal ("dota 2 beta") and must not prevent matching.
    VERIFY(windowsPathMatchesRule("S:\\common\\Deadlock\\game\\bin\\win64\\deadlock.exe -steam", deadlockDir, true));
    VERIFY(windowsPathMatchesRule("S:\\common\\dota 2 beta\\game\\bin\\linuxsteamrt64\\dota2", dotaDir, true));
    // Case-insensitive (Windows semantics), both separator styles, \\?\-prefixed.
    VERIFY(windowsPathMatchesRule("s:\\common\\deadlock\\GAME\\bin\\win64\\DEADLOCK.exe", deadlockDir, true));
    VERIFY(windowsPathMatchesRule("S:/common/Deadlock/game/bin/win64/deadlock.exe", deadlockDir, true));
    VERIFY(windowsPathMatchesRule("\\\\?\\Z:\\run\\host\\usr\\share\\wine\\xalia.exe",
                                  "/usr/share/wine", true));
    // Root-mapped drive without remap: the token is the absolute path with backslashes.
    VERIFY(windowsPathMatchesRule("\\\\?\\Z:\\mnt\\big\\SteamLibrary\\steamapps\\common\\Deadlock\\game\\bin\\win64\\deadlock.exe",
                                  deadlockDir, true));
    // ... and must not cross-match another game.
    VERIFY(!windowsPathMatchesRule("\\\\?\\Z:\\mnt\\big\\SteamLibrary\\steamapps\\common\\Deadlock\\game\\bin\\win64\\deadlock.exe",
                                   dotaDir, true));
    // Other wine processes of the same session must not match the game dir.
    VERIFY(!windowsPathMatchesRule("C:\\windows\\system32\\steam.exe", deadlockDir, true));
    VERIFY(!windowsPathMatchesRule("C:\\windows\\system32\\winedevice.exe", deadlockDir, true));
    // A different game in the same library must not match.
    VERIFY(!windowsPathMatchesRule("S:\\common\\dota 2 beta\\game\\bin\\linuxsteamrt64\\dota2", deadlockDir, true));
    // File rules: the token must be exactly the file, not a sibling under the same dir.
    VERIFY(windowsPathMatchesRule("S:\\common\\Deadlock\\game\\bin\\win64\\deadlock.exe", deadlockExeRule, false));
    VERIFY(!windowsPathMatchesRule("S:\\common\\Deadlock\\game\\bin\\win64\\other.exe", deadlockExeRule, false));
    // A single generic component cannot align (k >= 2 guard).
    VERIFY(!windowsPathMatchesRule("X:\\bin\\sh", "/usr/bin", true));
    // Non-Windows tokens are rejected outright.
    VERIFY(!windowsPathMatchesRule("/mnt/big/SteamLibrary/steamapps/common/Deadlock/game", deadlockDir, true));
    VERIFY(!windowsPathMatchesRule("deadlock.exe", deadlockDir, true));
    VERIFY(!windowsPathMatchesRule("", deadlockDir, true));

    VERIFY(stripRunHostPrefix("/run/host/usr/bin/foo") == "/usr/bin/foo");
    VERIFY(stripRunHostPrefix("/run/host") == "/run/host");            // no trailing component: unchanged
    VERIFY(stripRunHostPrefix("/run/hosto/usr/bin/foo") == "/run/hosto/usr/bin/foo"); // prefix must be a full component
    VERIFY(stripRunHostPrefix("/usr/bin/foo") == "/usr/bin/foo");

    // Live preloader exe observed via /proc on the user's machine.
    VERIFY(exeLooksLikeWineHost("/run/host/usr/share/steam/compatibilitytools.d/proton-cachyos-slr/files/lib/wine/x86_64-unix/wine64-preloader"));
    VERIFY(exeLooksLikeWineHost("/usr/bin/wineserver"));
    VERIFY(exeLooksLikeWineHost("/opt/lutris/.../files/bin/wine64"));
    // "wine" must match as a component prefix, not a substring anywhere.
    VERIFY(!exeLooksLikeWineHost("/usr/bin/rewinder"));
    VERIFY(!exeLooksLikeWineHost("/usr/bin/gwine"));
    VERIFY(!exeLooksLikeWineHost("/usr/bin/steam"));

    if (g_failures == 0) {
        printf("pathmatching.test: all checks passed\n");
    } else {
        printf("pathmatching.test: %d check(s) failed\n", g_failures);
    }
    return g_failures;
}
