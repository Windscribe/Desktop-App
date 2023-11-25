#include "wintuncontroller.h"

#include "logger.h"


WintunController::WintunController()
{
}

void WintunController::release()
{
    removeAdapter();
}

bool WintunController::createAdapter()
{
    removeAdapter();

    wintunDLL_ = ::LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    if (!wintunDLL_) {
        Logger::instance().out(L"Failed to load wintun.dll (%lu)", ::GetLastError());
        return false;
    }

    WINTUN_CREATE_ADAPTER_FUNC *createAdapter = (WINTUN_CREATE_ADAPTER_FUNC*)::GetProcAddress(wintunDLL_, "WintunCreateAdapter");
    if (createAdapter == NULL) {
        Logger::instance().out(L"Failed to resolve the WintunCreateAdapter function (%lu)", ::GetLastError());
        ::FreeLibrary(wintunDLL_);
        wintunDLL_ = nullptr;
        return false;
    }

    GUID wintunGuid = { 0xd8cf7bd4, 0x8b64, 0x4e57, { 0xb7, 0x07, 0x4c, 0x8b, 0xac, 0xbe, 0xc2, 0xc3 } };
    adapterHandle_ = createAdapter(adapterName().c_str(), L"Windscribe", &wintunGuid);
    if (!adapterHandle_) {
        Logger::instance().out(L"Failed to create the wintun adapter (%lu)", ::GetLastError());
        ::FreeLibrary(wintunDLL_);
        wintunDLL_ = nullptr;
        return false;
    }

    return true;
}

void WintunController::removeAdapter()
{
    if (wintunDLL_ == nullptr) {
        return;
    }

    if (adapterHandle_) {
        WINTUN_CLOSE_ADAPTER_FUNC *closeAdapter = (WINTUN_CLOSE_ADAPTER_FUNC*)::GetProcAddress(wintunDLL_, "WintunCloseAdapter");
        if (closeAdapter == NULL) {
            Logger::instance().out(L"Failed to resolve the WintunCloseAdapter function (%lu)", ::GetLastError());
        }
        else {
            closeAdapter(adapterHandle_);
            adapterHandle_ = nullptr;
        }
    }

    ::FreeLibrary(wintunDLL_);
    wintunDLL_ = nullptr;
}

std::wstring WintunController::adapterName()
{
    return std::wstring(L"Windscribe");
}
