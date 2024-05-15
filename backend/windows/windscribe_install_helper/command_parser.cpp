#include "command_parser.h"

#include <filesystem>

#include "install_service_command.h"

static std::wstring getInstallDir()
{
    wchar_t buffer[MAX_PATH];
    DWORD pathLen = ::GetModuleFileName(NULL, buffer, MAX_PATH);
    if (pathLen == 0) {
        Logger::WSDebugMessage(L"Windscribe install helper GetModuleFileName failed (%lu)", ::GetLastError());
        return std::wstring();
    }

    std::filesystem::path path(buffer);
    return path.parent_path().native();
}

CommandParser::CommandParser()
{
}

CommandParser::~CommandParser()
{
}

BasicCommand *CommandParser::parse(int argc, wchar_t* argv[])
{
    //	Command for install helper
    // /InstallService
    if (argc == 2 && wcscmp(argv[1], L"/InstallService") == 0) {
        const auto installDir = getInstallDir();
        if (!installDir.empty()) {
            Logger *logger = new Logger(installDir.c_str());
            InstallServiceCommand *cmd = new InstallServiceCommand(logger, installDir);
            return cmd;
        }
    }

    return nullptr;
}
