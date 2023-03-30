#include "install_splittunnel.h"

#include <sstream>

#include "../settings.h"
#include "../../../utils/directory.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/process1.h"
#include "../../../utils/redirection.h"
#include "../../../utils/services.h"

using namespace std;

InstallSplitTunnel::InstallSplitTunnel(double weight) : IInstallBlock(weight, L"splittunnel", false)
{
}

int InstallSplitTunnel::executeStep()
{
    wstring infFile = Path::AddBackslash(Settings::instance().getPath()) + wstring(L"splittunnel\\windscribesplittunnel.inf");
    wstring commandLine = wstring(L"setupapi,InstallHinfSection DefaultInstall 132 ") + infFile;

    Redirection redir;
    if (!redir.NewFileExists(infFile)) {
        Log::instance().out("WARNING: the split tunnel driver inf (%ls) was not found.", infFile.c_str());
        return -1;
    }

    // For a reason I was unable to determine, if this 64-bit installer is launched by the 32-bit app
    // (e.g. app version <= 2.4) it ends up running the sysWOW64 (32-bit) version of rundll32 which
    // incorrectly places the 64-bit split tunnel driver in the sysWOW64/drivers folder, but points
    // the service entry at the correct system32/drivers folder.  The helper is then unable to start
    // the driver.  Specifying the full path to the 64-bit rundll32 seems to resolve the issue.
    wstring appName = Directory::GetSystemDir() + wstring(L"\\rundll32.exe");

    for (int attempts = 1; attempts <= 2; ++attempts) {
        auto result = Process::InstExec(appName, commandLine, 30 * 1000, SW_HIDE);

        if (!result.has_value()) {
            Log::instance().out("WARNING: an error was encountered launching the split tunnel driver installer or while monitoring its progress.");
            return -1;
        }

        if (result.value() == WAIT_TIMEOUT) {
            Log::instance().out("WARNING: the split tunnel driver install stage timed out.");
            return -1;
        }

        if (result.value() != NO_ERROR) {
            Log::instance().out("WARNING: the split tunnel driver install returned a failure code (%lu).", result.value());
            return -1;
        }

        // Verify the service has been installed.  We've encountered cases where an uninstall followed rapidly by
        // an install (e.g. during an in-app upgrade) oddly causes the split tunnel driver install to silently fail.
        Services service;
        if (service.serviceExists(L"WindscribeSplitTunnel")) {
            break;
        }

        if (attempts == 1) {
            Log::instance().out("WARNING: the split tunnel service was not found after driver install, trying driver install again...");
            ::SleepEx(10000, FALSE);
        }
        else {
            Log::instance().out("WARNING: the split tunnel service is not installed.  The split tunneling feature will be disabled in the app.");
            return -1;
        }
    }

    return 100;
}
