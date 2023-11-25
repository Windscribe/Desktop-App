// ws_com_server
// This shell of a program is called via COM registry
// Though it doesn't do much by itself, having it allows us to prompt UAC for custom configuration change (since it is a separate process)

// links against ws_com.lib
#include <Windows.h>
#include <string>
#include "../ws_com/exports.h"

int main(int argc, char *argv[])
{
    // Prevent manual program startup
    // This program should only be runnable when called via COM/registry/SCM
    if ( argc < 2 ||
        !(std::string(argv[1]) == std::string("-Embedding") ||
          std::string(argv[1]) == std::string("/Embedding")))
    {
        // std::cout << "Not called from SCM, closing program" << std::endl;
        return 0;
    }

    CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    StartFactories();

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

