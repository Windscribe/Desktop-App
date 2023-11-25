// WindscribeInstallHelper.cpp : Defines the entry point for the console application.
//

#include <memory>
#include <tchar.h>

#include "command_parser.h"

int _tmain(int argc, _TCHAR* argv[])
{
    CommandParser parser;
    std::unique_ptr<BasicCommand> cmd(parser.parse(argc, argv));
    if (cmd) {
        cmd->execute();
    }
    return 0;
}
