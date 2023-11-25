// ChangeIcs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include "icsmanager.h"

// parse change command and return adapter name or empty string if failed
std::string parseChangeCommand(const std::string &cmd)
{
    size_t whitespacePos = cmd.find(' ');
    if (whitespacePos == std::string::npos)
        return std::string();

    std::string first = cmd.substr(0, whitespacePos);
    std::string second = cmd.substr(whitespacePos + 1);

    if (first != "change" || second.size() < 2)
        return std::string();

    if (second[0] != '\"' || second[second.size() - 1] != '\"')
        return std::string();

    return second.substr(1, second.size() - 2);
}

int _tmain(int argc, _TCHAR* argv[])
{
    CoInitializeEx(0, COINIT_MULTITHREADED);

    IcsManager icsManager;
    std::string command;

    // read commands from stdin, execute and print results in stdout
    while (std::getline(std::cin, command) && !command.empty()) {
        if (command == "is_supported") {
            if (icsManager.isSupported()) {
                std::cout << "true" << std::endl;
            } else {
                std::cout << "false" << std::endl;
            }
        } else if (command == "stop") {
            if (icsManager.stop()) {
                std::cout << "true" << std::endl;
            } else {
                std::cout << "false" << std::endl;
            }
            break;
        }
        // command change, for example 'change "Windsctibe VPN"'
        else if (command.rfind("change", 0) == 0) { // starts with prefix "change"?
            std::string adapterName = parseChangeCommand(command);
            if (!adapterName.empty()) {
                if (icsManager.change(std::wstring(adapterName.begin(), adapterName.end()))) {
                    std::cout << "true" << std::endl;
                } else {
                    std::cout << "false" << std::endl;
                }
            } else {
                std::cout << "false" << std::endl;
            }
        }
    }

    CoUninitialize();
    return 0;
}

