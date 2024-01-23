#include "ovpn.h"
#include "logger.h"
#include <fcntl.h>
#include <string>
#include <KnownFolders.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <string>

namespace OVPN
{

bool writeOVPNFile(std::wstring &filename, const std::wstring &config)
{
    // To prevent shenanigans with various TOCTOU exploits, write the config Program Files,
    // which is only writable by administrators
    std::wistringstream stream(config);
    wchar_t* programFilesPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &programFilesPath);
    if (FAILED(hr)) {
        Logger::instance().out("Failed to get Program Files dir");
        CoTaskMemFree(programFilesPath);
        return false;
    }
    std::wstringstream filePath;
    filePath << programFilesPath;
    CoTaskMemFree(programFilesPath);
    filePath << L"\\Windscribe\\config";
    int ret = SHCreateDirectoryEx(NULL, filePath.str().c_str(), NULL);
    if (ret != ERROR_SUCCESS && ret != ERROR_ALREADY_EXISTS) {
        Logger::instance().out("Failed to create config dir");
        return false;
    }

    filePath << L"\\config.ovpn";
    Logger::instance().out("Writing OpenVPN config");

    std::wofstream file(filePath.str().c_str(), std::ios::out | std::ios::trunc);
    if (!file) {
        Logger::instance().out("Could not open config file: %u", GetLastError());
        return false;
    }

    std::wstring line;
    while(getline(stream, line)) {
        // trim whitespace
        line.erase(0, line.find_first_not_of(L" \n\r\t"));
        line.erase(line.find_last_not_of(L" \n\r\t") + 1);

        // filter anything that runs an external script
        // check for up to offset of 2 in case the command starts with '--'
        if (line.rfind(L"up", 2) != std::string::npos ||
            line.rfind(L"tls-verify", 2) != std::string::npos ||
            line.rfind(L"ipchange", 2) != std::string::npos ||
            line.rfind(L"client-connect", 2) != std::string::npos ||
            line.rfind(L"route-up", 2) != std::string::npos ||
            line.rfind(L"route-pre-down", 2) != std::string::npos ||
            line.rfind(L"client-disconnect", 2) != std::string::npos ||
            line.rfind(L"down", 2) != std::string::npos ||
            line.rfind(L"learn-address", 2) != std::string::npos ||
            line.rfind(L"auth-user-pass-verify", 2) != std::string::npos)
        {
            continue;
        }
        file << line.c_str() << "\r\n";
    }
    file.close();

    filename = filePath.str();
    return true;
}

} // namespace OVPN
