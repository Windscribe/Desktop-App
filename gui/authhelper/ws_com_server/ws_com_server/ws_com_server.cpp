// ws_com_server
// This shell of a program i

// links against ws_com.lib
#include <Windows.h>
#include "../../ws_com/ws_com/exports.h"

int main()
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	StartFactories();

	// TODO: prevent manual spinup
	// TODO: prevent console opening

	// pump message loop
	MSG ms;
	while (GetMessage(&ms, 0, 0, 0) > 0)
	{
		TranslateMessage(&ms);
		DispatchMessage(&ms);
	}

	StopFactories();
	CoUninitialize();
	return 0;
}

