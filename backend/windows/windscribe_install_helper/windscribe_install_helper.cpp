// WindscribeInstallHelper.cpp : Defines the entry point for the console application.
//

#include <memory>
#include <tchar.h>

#include "command_parser.h"

// Set the DLL load directory to the system directory before entering WinMain().
struct LoadSystemDLLsFromSystem32
{
    LoadSystemDLLsFromSystem32()
    {
        // Remove the current directory from the search path for dynamically loaded
        // DLLs as a precaution.  This call has no effect for delay load DLLs.
        SetDllDirectory(L"");
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
} loadSystemDLLs;

int _tmain(int argc, _TCHAR* argv[])
{
    CommandParser parser;
    std::unique_ptr<BasicCommand> cmd(parser.parse(argc, argv));
    if (cmd) {
        cmd->execute();
    }
    return 0;
}
