#include "ws_branding.h"
#include "openvpncontroller.h"

#include <codecvt>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>

#include "types/global_consts.h"
#include "utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif

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

    return createTapAdapter();
}

void OpenVPNController::removeAdapter()
{
    if (useDCODriver_) {
        removeDCOAdapter();
    } else if (tapAdapterCreated_) {
        removeTapAdapter();
    }
}

bool OpenVPNController::createTapAdapter()
{
    useDCODriver_ = false;
    tapAdapterCreated_ = true;

    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" create --name " << kOpenVPNAdapterIdentifier << L" --hwid root\\tap0901";

    spdlog::info(L"createTapAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    if (result.success && result.exitCode != 0) {
        spdlog::error(L"createTapAdapter cmd failed: {}", result.output);
    }

    return result.success;
}

void OpenVPNController::removeTapAdapter()
{
    tapAdapterCreated_ = false;
    std::wstringstream stream;
    stream << L"\"" << Utils::getExePath() << L"\\tapctl.exe\"" << L" delete " << kOpenVPNAdapterIdentifier;

    spdlog::info(L"removeTapAdapter cmd = {}", stream.str());
    auto result = ExecuteCmd::instance().executeBlockingCmd(stream.str());

    if (result.success && result.exitCode != 0) {
        spdlog::error(L"removeTapAdapter cmd failed: {}", result.output);
    }
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

    const std::wstring cwd = Utils::getDirPathFromFullPath(filename);
    const std::wstring strCmd = L"\"" + ovpnExe + L"\" --config \"" + filename + L"\"";
    return ExecuteCmd::instance().executeNonblockingCmd(strCmd, cwd);
}

bool OpenVPNController::writeOVPNFile(std::wstring &config, unsigned int port, const std::wstring &httpProxy, unsigned int httpPort,
                                      const std::wstring &socksProxy, unsigned int socksPort, std::wstring &filename)
{
    // Replace the deprecated (as of OpenVPN 2.5) ciphers flag received from the server API with the proper one.
    boost::replace_all(config, L"ncp-ciphers", L"data-ciphers");

    if (useDCODriver_) {
        // DCO driver on Windows will not accept the AES-256-CBC cipher and will drop back to using wintun if it is provided in the ciphers list.
        // TODO: implement a smarter algorithm for replacement here in case the list of ciphers received from the server API ever changes.
        boost::ireplace_all(config, L":AES-256-CBC:", L":");
    }

    std::string narrowConfig;
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        narrowConfig = converter.to_bytes(config);
    } catch (const std::range_error &) {
        spdlog::error("OpenVPN config is not valid UTF-16");
        return false;
    }

    std::wstring filePath = Utils::getConfigPath();
    if (filePath.empty()) {
        spdlog::error("Could not get config path");
        return false;
    }

    filePath += L"\\config.ovpn";

    // Remove any prior config before filtering so a rejected config never leaves a stale file behind.
    DeleteFileW(filePath.c_str());

    std::string filtered;
    if (!OvpnDirectiveWhitelist::filterConfig(narrowConfig, filtered)) {
        return false;
    }

    spdlog::debug("Writing OpenVPN config");

    std::wofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file) {
        spdlog::error("Could not open config file: {}", GetLastError());
        return false;
    }

    // Write filtered config (filterConfig outputs \n; Windows needs \r\n but
    // OpenVPN handles both, and the helper-appended options below use \r\n)
    file << filtered.c_str();

    // add management and other options
    file << L"management 127.0.0.1 " + std::to_wstring(port) + L"\r\n";
    file << L"management-query-passwords\r\n";
    file << L"management-hold\r\n";
    file << L"verb 3\r\n";

    // We use the --dev-node option to ensure OpenVPN will only use the DCO/wintun adapter instance we create and not possibly
    // attempt to use an adapter created by other software (e.g. the vanilla OpenVPN client app).
    file << L"dev tun\r\n";
    file << L"dev-node " + std::wstring(kOpenVPNAdapterIdentifier) + L"\r\n";
    // OpenVPN 2.7 removed --windows-driver; the driver is now auto-selected from DCO availability.
    // For the non-DCO path we must explicitly disable DCO, otherwise OpenVPN picks DCO whenever the
    // ovpn-dco-win driver is loaded on the system (e.g. left over from a prior DCO connection) and
    // then bails with an adapter-driver-mismatch fatal against our tap-windows6 adapter.
    if (!useDCODriver_) {
        file << L"disable-dco\r\n";

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

    if (config.find(L"remote 127.0.0.1") != std::wstring::npos) {
        file << L"pull-filter ignore \"redirect-gateway\"\r\n";
        file << L"route 0.0.0.0 128.0.0.0 vpn_gateway\r\n";
        file << L"route 128.0.0.0 128.0.0.0 vpn_gateway\r\n";
    }

    file.close();

    filename = filePath;
    return true;
}
