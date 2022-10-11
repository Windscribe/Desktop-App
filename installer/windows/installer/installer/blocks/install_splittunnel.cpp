#include "install_splittunnel.h"

#include <sstream>

#include "../settings.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/process1.h"

using namespace std;

InstallSplitTunnel::InstallSplitTunnel(double weight) : IInstallBlock(weight, L"splittunnel", false)
{
}

int InstallSplitTunnel::executeStep()
{
    wostringstream commandLine;
    commandLine << L"setupapi,InstallHinfSection DefaultInstall 132 " << Path::AddBackslash(Settings::instance().getPath()) << L"splittunnel\\windscribesplittunnel.inf";

    auto result = Process::InstExec(L"rundll32", commandLine.str(), 30 * 1000, SW_HIDE);

    if (!result.has_value()) {
        Log::instance().out("WARNING: an error was encountered launching the split tunnel driver installer or while monitoring its progress.");
        return -1;
    }

    if (result.value() == WAIT_TIMEOUT) {
        Log::instance().out("WARNING: The split tunnel driver install stage timed out.");
        return -1;
    }

    if (result.value() != NO_ERROR) {
        Log::instance().out("WARNING: The split tunnel driver install returned a failure code (%lu).", result.value());
        return -1;
    }

    return 100;
}
