// WindscribeInstallHelper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "command_parser.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CommandParser parser;
	BasicCommand *cmd = parser.parse(argc, argv);
	if (cmd)
	{
		cmd->execute();
		delete cmd;
	}
	return 0;
}

