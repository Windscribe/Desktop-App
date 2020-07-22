#include "Server.h"
#include <iostream>
#include <boost/thread/thread.hpp>

enum LAST_CMD { LAST_CMD_TRUE, LAST_CMD_FALSE, LAST_CMD_NONE };

LAST_CMD lastCmd = LAST_CMD_NONE;

bool serverCallback(std::string authHash, bool isConnectCmd, bool isDisconnectCmd, std::string location)
{
	printf("CMD: %s,%d,%d,%s\n", authHash.c_str(), isConnectCmd, isDisconnectCmd, location.c_str());
	fflush(stdout);

	// wait for answer (maximum 10 sec)
    int64_t beginTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	while (true)
	{
		if (lastCmd == LAST_CMD_TRUE)
		{
			lastCmd = LAST_CMD_NONE;
			return true;
		}
		else if (lastCmd == LAST_CMD_FALSE)
		{
			lastCmd = LAST_CMD_NONE;
			return false;
        }
#ifdef Q_OS_WIN
        Sleep(1);
#else
        usleep(1000);
#endif

        int64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if ((curTime - beginTime) > 10000)
		{
			lastCmd = LAST_CMD_NONE;
			return false;
		}
	}

	return false;
}

int main(int argc, char* argv[])
{
    // hide unused variables warnings
    (void)argc;
    (void)argv;

    Server server(serverCallback);

	std::vector<unsigned int> ports;
	ports.push_back(13337);
	ports.push_back(23337);
	ports.push_back(33337);
	server.start(ports);

    std::string inputStr;

    try
    {
        while (std::getline (std::cin, inputStr))
        {
            if (inputStr == "exit")
            {
                break;
            }
            else if (inputStr == "true")
            {
                lastCmd = LAST_CMD_TRUE;
            }
            else if (inputStr == "false")
            {
                lastCmd = LAST_CMD_FALSE;
            }
        }
    }
    catch(...) { }
	return 0;
}

