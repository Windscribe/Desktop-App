#include "openvpncontroller.h"

#include <sstream>
#include <spdlog/spdlog.h>

#include "executecmd.h"
#include "types/global_consts.h"
#include "utils.h"


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
        spdlog::error("createDCOAdapter cmd failed: {}", result.additionalString);
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
        spdlog::error("removeDCOAdapter cmd failed: {}", result.additionalString);
    }
}
