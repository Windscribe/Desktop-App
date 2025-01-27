#include "shellexecuteasuser.h"

#include <atlbase.h>
#include <shlobj.h>
#include <wrl/client.h>

#include <system_error>

#include "wsscopeguard.h"

namespace wsl {

void RunDeElevated(const std::wstring &path)
{
    HRESULT hr = ::CoInitialize(nullptr);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated CoInitialize failed");
    }

    auto exitGuard = wsl::wsScopeGuard([&] {
        ::CoUninitialize();
    });

    Microsoft::WRL::ComPtr<IShellWindows> shell;
    hr = ::CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&shell));
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated CoCreateInstance(CLSID_ShellWindows) failed");
    }

    long hwnd = 0;
    CComVariant vtLoc(CSIDL_DESKTOP);
    CComVariant vtEmpty;
    Microsoft::WRL::ComPtr<IDispatch> dispatch;
    hr = shell->FindWindowSW(&vtLoc, &vtEmpty, SWC_DESKTOP, &hwnd, SWFO_NEEDDISPATCH, &dispatch);
    if (hr == S_FALSE || FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated FindWindowSW failed");
    }

    Microsoft::WRL::ComPtr<IServiceProvider> service;
    hr = dispatch.As(&service);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated IServiceProvider failed");
    }

    Microsoft::WRL::ComPtr<IShellBrowser> browser;
    hr = service->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser));
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated IShellBrowser failed");
    }

    Microsoft::WRL::ComPtr<IShellView> view;
    hr = browser->QueryActiveShellView(&view);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated IShellView failed");
    }

    hr = view->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&dispatch));
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated GetItemObject failed");
    }

    Microsoft::WRL::ComPtr<IShellFolderViewDual> folder;
    hr = dispatch.As(&folder);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated IShellFolderViewDual failed");
    }

    hr = folder->get_Application(&dispatch);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated get_Application failed");
    }

    Microsoft::WRL::ComPtr<IShellDispatch2> shell_dispatch;
    hr = dispatch.As(&shell_dispatch);
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated IShellDispatch2 failed");
    }

    hr = shell_dispatch->ShellExecute(CComBSTR(path.c_str()), CComVariant(L""), CComVariant(L""), CComVariant(L""), CComVariant(SW_RESTORE));
    if (FAILED(hr)) {
        throw std::system_error(HRESULT_CODE(hr), std::system_category(), "RunDeElevated ShellExecute failed");
    }
}

}
