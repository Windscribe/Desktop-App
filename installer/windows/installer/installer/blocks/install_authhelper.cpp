#include "install_authhelper.h"

#include <spdlog/spdlog.h>

#include "installerenums.h"
#include "../settings.h"
#include "../../../Utils/path.h"

InstallAuthHelper::InstallAuthHelper(double weight) : IInstallBlock(weight, L"AuthHelper")
{
}

InstallAuthHelper::~InstallAuthHelper()
{

}

int InstallAuthHelper::executeStep()
{
    const std::wstring& installPath = Settings::instance().getPath();

    // First register the proxy stub lib
    std::wstring authProxyStubLib = Path::append(installPath, L"ws_proxy_stub.dll");

    // Intentionally linking via "LoadLibrary" method to reduce compile/static link time dependencies:
    // * authhelper ws_com exports.h
    // * authhelper ws_com ws_com.lib
    // We still require run-time dynamic linking with DLL at executable location
    HINSTANCE hProxyStubLib = LoadLibrary(authProxyStubLib.c_str());
    if (hProxyStubLib == NULL) {
        spdlog::error(L"Failed to load Auth Helper Proxy Stub Library");
        lastError_ = L"An error occurred when loading the Auth Helper Proxy Stub library";
        return -wsl::ERROR_OTHER;
    }

    typedef HRESULT(*simpleFunc) (void);
    simpleFunc DllRegisterServer = (simpleFunc)::GetProcAddress(hProxyStubLib, "DllRegisterServer");

    if (DllRegisterServer != NULL) {
        HRESULT result = DllRegisterServer();
        if (FAILED(result)) {
            spdlog::error(L"Call to Proxy Stub DllRegisterServer failed");
            lastError_ = L"An error occurred when calling Proxy Stub DllRegisterServer";
            FreeLibrary(hProxyStubLib);
            return -wsl::ERROR_OTHER;
        }
    } else {
        spdlog::error(L"Failed to get proxy stub DllRegisterServer");
        lastError_ = L"An error occurred when getting proxy stub DllRegisterServer";
        FreeLibrary(hProxyStubLib);
        return -wsl::ERROR_OTHER;
    }
    FreeLibrary(hProxyStubLib);


    // Register the ws_com lib
    std::wstring authLib = Path::append(installPath, L"ws_com.dll");

    HINSTANCE hLib = LoadLibrary(authLib.c_str());
    if (hLib == NULL) {
        spdlog::error(L"Failed to load Auth Helper Library");
        lastError_ = L"An error occurred when loading the Auth Helper library: ";
        return -wsl::ERROR_OTHER;
    }

    typedef HRESULT(__stdcall *someFunc) (const std::wstring &, const std::wstring &, const std::wstring &);
    someFunc RegisterServerWithTargetPaths = (someFunc)::GetProcAddress(hLib, "RegisterServerWithTargetPaths");

    if (RegisterServerWithTargetPaths != NULL) {

        HRESULT result = RegisterServerWithTargetPaths(installPath, installPath, installPath);

        if (FAILED(result)) {
            spdlog::error(L"Call to RegisterServerWithTargetPaths failed");
            lastError_ = L"An error occurred when calling RegisterServerWithTargetPaths: ";
            FreeLibrary(hLib);
            return -wsl::ERROR_OTHER;
        }
    } else {
        spdlog::error(L"Failed to get reg server function");
        lastError_ = L"An error occurred when getting RegisterServerWithTargetPaths: ";
        FreeLibrary(hLib);
        return -wsl::ERROR_OTHER;
    }
    FreeLibrary(hLib);
    spdlog::info(L"Auth helper installed successfully");
    return 100;
}
