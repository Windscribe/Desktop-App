#include "openvpncontroller.h"

#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>

#include "types/global_consts.h"
#include "utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif


OpenVPNController::OpenVPNController()
{
}

void OpenVPNController::release()
{
    removeAdapter();
}

bool OpenVPNController::createAdapter(bool useDCODriver)
{
    removeAdapter();

    if (useDCODriver) {
        return createDCOAdapter();
    }

    return createWintunAdapter();
}

void OpenVPNController::removeAdapter()
{
    if (useDCODriver_) {
        removeDCOAdapter();
    } else {
        removeWintunAdapter();
    }
}

bool OpenVPNController::createWintunAdapter()
{
    useDCODriver_ = false;

    wintunDLL_ = ::LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    if (!wintunDLL_) {
        spdlog::error("Failed to load wintun.dll ({})", ::GetLastError());
        return false;
    }

    WINTUN_CREATE_ADAPTER_FUNC *createAdapter = (WINTUN_CREATE_ADAPTER_FUNC*)::GetProcAddress(wintunDLL_, "WintunCreateAdapter");
    if (createAdapter == NULL) {
        spdlog::error("Failed to resolve the WintunCreateAdapter function ({})", ::GetLastError());
        ::FreeLibrary(wintunDLL_);
        wintunDLL_ = nullptr;
        return false;
    }

    GUID wintunGuid = { 0xd8cf7bd4, 0x8b64, 0x4e57, { 0xb7, 0x07, 0x4c, 0x8b, 0xac, 0xbe, 0xc2, 0xc3 } };
    adapterHandle_ = createAdapter(kOpenVPNAdapterIdentifier, L"Windscribe Wintun", &wintunGuid);
    if (!adapterHandle_) {
        spdlog::error("Failed to create the wintun adapter ({})", ::GetLastError());
        ::FreeLibrary(wintunDLL_);
        wintunDLL_ = nullptr;
        return false;
    }

    return true;
}

void OpenVPNController::removeWintunAdapter()
{
    if (wintunDLL_ == nullptr) {
        return;
    }

    if (adapterHandle_) {
        WINTUN_CLOSE_ADAPTER_FUNC *closeAdapter = (WINTUN_CLOSE_ADAPTER_FUNC*)::GetProcAddress(wintunDLL_, "WintunCloseAdapter");
        if (closeAdapter == NULL) {
            spdlog::error("Failed to resolve the WintunCloseAdapter function ({})", ::GetLastError());
        }
        else {
            closeAdapter(adapterHandle_);
            adapterHandle_ = nullptr;
        }
    }

    ::FreeLibrary(wintunDLL_);
    wintunDLL_ = nullptr;
}

bool OpenVPNController::createDCOAdapter()
{
    useDCODriver_ = true;

    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" create --name " << kOpenVPNAdapterIdentifier << L" --hwid ovpn-dco";

    spdlog::info(L"createDCOAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    if (result.success && result.exitCode != 0) {
        spdlog::error(L"createDCOAdapter cmd failed: {}", result.output);
    }

    return result.success;
}

void OpenVPNController::removeDCOAdapter()
{
    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" delete " << kOpenVPNAdapterIdentifier;

    spdlog::info(L"removeDCOAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    if (result.success && result.exitCode != 0) {
        spdlog::error(L"removeDCOAdapter cmd failed: {}", result.output);
    }
}

ExecuteCmdResult OpenVPNController::runOpenvpn(std::wstring &config, unsigned int port, const std::wstring &httpProxy,
                                               unsigned int httpPort, const std::wstring &socksProxy, unsigned int socksPort)
{
    std::wstring filename;
    if (!writeOVPNFile(config, port, httpProxy, httpPort, socksProxy, socksPort, filename)) {
        return ExecuteCmdResult();
    }

    const std::wstring ovpnExe = Utils::getExePath() + L"\\windscribeopenvpn.exe";

#if defined(USE_SIGNATURE_CHECK)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ovpnExe)) {
        spdlog::error("OpenVPN service signature incorrect: {}", sigCheck.lastError());
        return ExecuteCmdResult();
    }
#endif

    std::wstring strCmd = L"\"" + ovpnExe + L"\" --config \"" + filename + L"\"";
    return ExecuteCmd::instance().executeUnblockingCmd(strCmd, L"", Utils::getDirPathFromFullPath(filename));
}

bool OpenVPNController::writeOVPNFile(std::wstring &config, unsigned int port, const std::wstring &httpProxy, unsigned int httpPort,
                                      const std::wstring &socksProxy, unsigned int socksPort, std::wstring &filename)
{
    // Replace the deprecated (as of OpenVPN 2.5) ciphers flag received from the server API with the proper one.
    boost::replace_all(config, L"ncp-ciphers", L"data-ciphers");

    if (useDCODriver_) {
        // DCO driver on Windows will not accept the AES-256-CBC cipher and will drop back to using wintun if it is provided in the ciphers list.
        // TODO: implement a smarter algorithm for replacement here in case the list of ciphers received from the server API ever changes.
        boost::replace_all(config, L":AES-256-CBC:", L":");
    }

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
    while (getline(stream, line)) {
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

    // The --dev tun option is already included in ovpnData by the server API.
    // We use the --dev-node option to ensure OpenVPN will only use the DCO/wintun adapter instance we create and not possibly
    // attempt to use an adapter created by other software (e.g. the vanilla OpenVPN client app).
    file << L"dev-node " + std::wstring(kOpenVPNAdapterIdentifier) + L"\r\n";
    if (useDCODriver_) {
        file << L"windows-driver ovpn-dco\r\n";
    } else {
        file << L"windows-driver wintun\r\n";

        // OpenVPN requires us to use the wintun driver when specifying a proxy setting.  The app should have requested us to
        // create the appropriate driver based on the user's settings prior to requesting we start the OpenVPN daemon.
        if (httpProxy.length() > 0) {
            if (!Utils::hasWhitespaceInString(httpProxy)) {
                file << L"http-proxy " + httpProxy + L" " + std::to_wstring(httpPort) + L" auto\r\n";
            }
        } else if (socksProxy.length() > 0) {
            if (!Utils::hasWhitespaceInString(socksProxy)) {
                file << L"socks-proxy " + socksProxy + L" " + std::to_wstring(socksPort) + L"\r\n";
            }
        }
    }

    file.close();

    filename = filePath;
    return true;
}
