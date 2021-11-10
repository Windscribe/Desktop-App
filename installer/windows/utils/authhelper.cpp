#include "authhelper.h"
#include <Windows.h>
#include "path.h"
#include "logger.h"

namespace AuthHelper {

bool removeRegEntriesForAuthHelper(const std::wstring &installPath)
{
	std::wstring authLib = Path::AddBackslash(installPath) + L"ws_com.dll";

	HINSTANCE hLib = LoadLibrary(authLib.c_str());
	if (hLib == NULL)
	{
		Log::instance().out(L"Failed to load ws_com.dll");
		return false;
	}

	// Note: ws_com.dll DllUnregisterServer will also remove the proxy-stub reg entries, so there is no need to run the proxy-stub DllUnregisterServer
	typedef HRESULT(*someFunc) (void);
	someFunc DllUnregisterServer = (someFunc)::GetProcAddress(hLib, "DllUnregisterServer");

	if (DllUnregisterServer != NULL)
	{
		HRESULT result = DllUnregisterServer();

		if (FAILED(result))
		{
			Log::instance().out(L"Call to DllUnregisterServer failed");
			FreeLibrary(hLib);
			return false;
		}
	}
	else
	{
		Log::instance().out(L"Failed to get proc DllUnregisterServer");
		FreeLibrary(hLib);
		return false;
	}

	FreeLibrary(hLib);
	return true;
}


} // name