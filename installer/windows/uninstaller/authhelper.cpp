#include "authhelper.h"

#include <Windows.h>

#include "../utils/logger.h"
#include "../utils/path.h"
#include "wsscopeguard.h"

namespace AuthHelper {

bool removeRegEntriesForAuthHelper(const std::wstring &installPath)
{
    std::wstring authLib = Path::append(installPath, L"ws_com.dll");

    HINSTANCE hLib = LoadLibrary(authLib.c_str());
    if (hLib == NULL) {
        Log::instance().out(L"Failed to load ws_com.dll");
        return false;
    }

    auto exitGuard = wsl::wsScopeGuard([&] {
        ::FreeLibrary(hLib);
    });

    // Note: ws_com.dll DllUnregisterServer will also remove the proxy-stub reg entries, so there is no need to run the proxy-stub DllUnregisterServer
    typedef HRESULT(*someFunc) (void);
    someFunc DllUnregisterServer = (someFunc)::GetProcAddress(hLib, "DllUnregisterServer");

    if (DllUnregisterServer == NULL) {
        Log::instance().out(L"Failed to get proc DllUnregisterServer from ws_com.dll");
        return false;
    }

    HRESULT result = DllUnregisterServer();

    if (FAILED(result)) {
        Log::instance().out(L"Call to ws_com DllUnregisterServer failed");
        return false;
    }

    return true;
}

}
