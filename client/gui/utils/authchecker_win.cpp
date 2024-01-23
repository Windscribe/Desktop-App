#include "authchecker_win.h"

#include <QCoreApplication>
#include <QScopeGuard>

#include <strsafe.h>

#include "utils/executable_signature/executable_signature.h"
#include "utils/logger.h"
#include "utils/winutils.h"

#include "../../gui/authhelper/win/ws_com/guids.h"

static HRESULT CoCreateInstanceAsAdmin(HWND hwnd, REFCLSID rclsid, REFIID riid, __out void ** ppv)
{
    BIND_OPTS3 bo;
    WCHAR  wszCLSID[50];
    WCHAR  wszMonikerName[300];

    StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID) / sizeof(wszCLSID[0]));
    HRESULT hr = StringCchPrintf(wszMonikerName, sizeof(wszMonikerName) / sizeof(wszMonikerName[0]), L"Elevation:Administrator!new:%s", wszCLSID);
    if (FAILED(hr))
        return hr;
    // std::wcout << L"Moniker name: " << wszMonikerName << std::endl;

    memset(&bo, 0, sizeof(bo));
    bo.cbStruct = sizeof(bo);
    bo.hwnd = hwnd;
    bo.dwClassContext = CLSCTX_LOCAL_SERVER;
    return CoGetObject(wszMonikerName, &bo, riid, ppv);
}

static bool authorizeWithUac()
{
    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    IUnknown *pThing = NULL;
    auto comRelease = qScopeGuard([&] {
        if (pThing != NULL) {
            pThing->Release();
        }

        CoUninitialize();
    });


    HRESULT hr = CoCreateInstanceAsAdmin(NULL, CLSID_AUTH_HELPER, IID_AUTH_HELPER, (void**)&pThing);
    if (FAILED(hr)) {
        int errorCode = HRESULT_CODE(hr);
        if (errorCode == ERROR_CANCELLED) {
            qCDebug(LOG_AUTH_HELPER) << "Authentication failed due to user selection";
        }
        else {
            // Can fail here if StubProxyDll isn't in CLSID\InprocServer32
            int facility = HRESULT_FACILITY(hr); // If returns 4 (FACILITY_ITF) then error codes are interface specific
            wchar_t strErr[1024];
            WinUtils::Win32GetErrorString(errorCode, strErr, _countof(strErr));
            qCDebug(LOG_AUTH_HELPER) << "Failed to CoCreateInstance of MyThing, facility: " << facility << ", code: " << errorCode;
            qCDebug(LOG_AUTH_HELPER) << " (" << hr << "): " << strErr;
        }

        return false;
    }

    // CoCreateInstanceAsAdmin will return S_OK if authorization was successful
    qCDebug(LOG_AUTH_HELPER) << "Helper process is Authorized";
    return true;
}

AuthChecker_win::AuthChecker_win(QObject *parent) : IAuthChecker(parent)
{
}

AuthCheckerError AuthChecker_win::authenticate()
{
    QString appDir = QCoreApplication::applicationDirPath();

    ExecutableSignature sigCheck;

    QString comServerPath = appDir + "/ws_com_server.exe";
    if (!sigCheck.verify(comServerPath.toStdWString())) {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comServerPath << ". File may be corrupted. " << QString::fromStdString(sigCheck.lastError());
        return AuthCheckerError::AUTH_HELPER_ERROR;
    }

    QString comDllPath    = appDir + "/ws_com.dll";
    if (!sigCheck.verify(comDllPath.toStdWString())) {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comDllPath << ". File may be corrupted. " << QString::fromStdString(sigCheck.lastError());
        return AuthCheckerError::AUTH_HELPER_ERROR;
    }

    QString comStubPath   = appDir + "/ws_proxy_stub.dll";
    if (!sigCheck.verify(comStubPath.toStdWString())) {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comStubPath << ". File may be corrupted. " << QString::fromStdString(sigCheck.lastError());
        return AuthCheckerError::AUTH_HELPER_ERROR;
    }

    return authorizeWithUac() ? AuthCheckerError::AUTH_NO_ERROR : AuthCheckerError::AUTH_AUTHENTICATION_ERROR;
}
