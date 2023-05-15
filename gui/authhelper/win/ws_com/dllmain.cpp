// ws_com is used by auth_helper for:
// * COM class object storage (used by com_server.exe)
// * Easier registry debugging (un/install reg entries via regsvr32.exe)
// * Simplifying installer for reg entry installs (installed links to this dll)
// * Resource storage for UAC (text and icon)
#include "Windows.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

