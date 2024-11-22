#include "ovpn.h"
#include <spdlog/spdlog.h>
#include <fcntl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <string>

#include "utils.h"

namespace OVPN
{

bool writeOVPNFile(std::wstring &filename, int port, const std::wstring &config, const std::wstring &httpProxy, int httpPort, const std::wstring &socksProxy, int socksPort)
{
    std::wistringstream stream(config);
    std::wstring filePath = Utils::getConfigPath();
    if (filePath.empty()) {
        spdlog::error("Could not get config path");
        return false;
    }

    filePath += L"\\config.ovpn";
    spdlog::debug("Writing OpenVPN config");

    std::wofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file) {
        spdlog::error("Could not open config file: {}", GetLastError());
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
            line.rfind(L"auth-user-pass-verify", 2) != std::string::npos ||
            line.rfind(L"management", 2) != std::string::npos ||
            line.rfind(L"http-proxy", 2) != std::string::npos ||
            line.rfind(L"socks-proxy", 2) != std::string::npos)
        {
            continue;
        }
        file << line.c_str() << L"\r\n";
    }

    // add management and other options
    file << L"management 127.0.0.1 " + std::to_wstring(port) + L"\r\n";
    file << L"management-query-passwords\r\n";
    file << L"management-hold\r\n";
    file << L"verb 3\r\n";

    if (httpProxy.length() > 0) {
        file << L"http-proxy " + httpProxy + L" " + std::to_wstring(httpPort) + L" auto\r\n";
    } else if (socksProxy.length() > 0) {
        file << L"socks-proxy " + socksProxy + L" " + std::to_wstring(socksPort) + L"\r\n";
    }

    file.close();

    filename = filePath;
    return true;
}

} // namespace OVPN
