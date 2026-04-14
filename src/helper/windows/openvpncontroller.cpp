#include "ws_branding.h"
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

#include <codecvt>

#include "../common/ovpn_directive_whitelist.h"


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
    adapterHandle_ = createAdapter(kOpenVPNAdapterIdentifier, WS_PRODUCT_NAME_W L" Wintun", &wintunGuid);
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

    const std::wstring ovpnExe = Utils::getExePath() + L"\\" WS_PRODUCT_NAME_LOWER_W L"openvpn.exe";

#if defined(USE_SIGNATURE_CHECK)
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(ovpnExe)) {
        spdlog::error("OpenVPN service signature incorrect: {}", sigCheck.lastError());
        return ExecuteCmdResult();
    }
#endif

    std::wstring strCmd = L"\"" + ovpnExe + L"\" --config \"" + filename + L"\"";
    return ExecuteCmd::instance().executeNonblockingCmd(strCmd, Utils::getDirPathFromFullPath(filename));
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

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    const std::string narrowConfig = converter.to_bytes(config);
    std::string filtered = OvpnDirectiveWhitelist::filterConfig(narrowConfig,
        [](const std::string &line) { spdlog::warn("Blocked non-whitelisted OpenVPN directive: {}", line); });

    // Write filtered config (filterConfig outputs \n; Windows needs \r\n but
    // OpenVPN handles both, and the helper-appended options below use \r\n)
    file << filtered.c_str();

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
