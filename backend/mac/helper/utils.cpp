#include "utils.h"
#include "3rdparty/pstream.h"
#include "logger.h"
#include <sstream>

namespace Utils
{

// based on 3rd party lib (http://pstreams.sourceforge.net/)
int executeCommand(const std::string &cmd, const std::vector<std::string> &args,
                   std::string *pOutputStr, bool appendFromStdErr)
{
    std::string cmdLine = cmd;

    for (auto it = args.begin(); it != args.end(); ++it)
    {
        cmdLine += " \"";
        cmdLine += *it;
        cmdLine += "\"";
    }

    if (pOutputStr)
    {
        pOutputStr->clear();
    }

    redi::ipstream proc(cmdLine, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
    {
        if (pOutputStr)
        {
            *pOutputStr += line + "\n";
        }
    }
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
    {
        proc.clear();
    }

    if (appendFromStdErr) {
        // read child's stderr
        while (std::getline(proc.err(), line))
        {
            if (pOutputStr)
            {
                *pOutputStr += line + "\n";
            }
        }
    }

    proc.close();
    if (proc.rdbuf()->exited())
    {
         return proc.rdbuf()->status();
    }
    return 0;
}


size_t findCaseInsensitive(std::string data, std::string toSearch, size_t pos)
{
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);
    return data.find(toSearch, pos);
}

std::string getFullCommand(const std::string &exePath, const std::string &executable, const std::string &arguments)
{
    char *canonicalPath = realpath(exePath.c_str(), NULL);
    if (!canonicalPath) {
        LOG("Executable not in valid path, ignoring.");
        return "";
    }

#if defined(USE_SIGNATURE_CHECK)
    if (std::string(canonicalPath).rfind("/Applications/Windscribe.app", 0) != 0) {
        // Don't execute arbitrary commands, only executables that are in our application directory
        LOG("Executable not in correct path, ignoring.");
        free(canonicalPath);
        return "";
    }
#endif

    std::string fullCmd = std::string(canonicalPath) + "/" + executable + " " + arguments;
    LOG("Resolved command: %s", fullCmd.c_str());
    free(canonicalPath);

    if (fullCmd.find(";") != std::string::npos || fullCmd.find("|") != std::string::npos || fullCmd.find("&") != std::string::npos) {
        // Don't execute commands with dangerous pipes or delimiters
        LOG("Executable command contains invalid characters, ignoring.");
        return "";
    }

    return fullCmd;
}

std::string getFullCommandAsUser(const std::string &user, const std::string &exePath, const std::string &executable, const std::string &arguments)
{
    return "sudo -u " + user + " " + getFullCommand(exePath, executable, arguments);
}

std::vector<std::string> getOpenVpnExeNames()
{
    std::vector<std::string> ret;
    std::string list;
    std::string item;
    int rc = 0;

    rc = Utils::executeCommand("ls", {"/Applications/Windscribe.app/Contents/Helpers"}, &list);
    if (rc != 0) {
        return ret;
    }

    std::istringstream stream(list);
    while (std::getline(stream, item)) {
        if (item.find("openvpn") != std::string::npos) {
            ret.push_back(item);
        }
    }
    return ret;
}

void createWindscribeUserAndGroup()
{
    bool exists = !Utils::executeCommand("id windscribe");
    if (exists) {
        // set the user to hidden if it wasn't already
        Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "IsHidden", "1"});
        return;
    }

    // Create group
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe"});
    // Below attributes are required for group to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "gid", "518"}); // Arbitrary number
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "GroupMembership", "windscribe"});
    Utils::executeCommand("dscl", {".", "-create", "/Groups/windscribe", "RealName", "Windscribe Apps Group"});

    // Create user
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "IsHidden", "1"});
    // Below attributes are required for user to be considered valid
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "gid", "518"}); // From above
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "uid", "1639"}); // Arbitrary number
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "passwd", "*"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "RealName", "Windscribe Apps User"});
    Utils::executeCommand("dscl", {".", "-create", "/Users/windscribe", "UserShell", "/bin/false"});
}

bool isAppUninstalled()
{
    // App is uninstalled if the application path is no longer valid AND the installer app is not running
    return Utils::executeCommand("ls", {"/Applications/Windscribe.app"}) && Utils::executeCommand("pgrep", {"-f", "WindscribeInstaller.app"});
}

void deleteSelf()
{
    Utils::executeCommand("launchctl", {"remove", "/Library/LaunchDaemons/com.windscribe.helper.macos.plist"});
    Utils::executeCommand("rm", {"/Library/LaunchDaemons/com.windscribe.helper.macos.plist"});
    Utils::executeCommand("rm", {"/Library/PrivilegedHelperTools/com.windscribe.helper.macos"});
    Utils::executeCommand("rm", {"/usr/local/bin/windscribe-cli"});
    Utils::executeCommand("dscl", {".", "-delete", "/Users/windscribe"});
    Utils::executeCommand("dscl", {".", "-delete", "/Groups/windscribe"});
}

} // namespace Utils
