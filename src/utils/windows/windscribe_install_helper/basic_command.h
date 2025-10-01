#pragma once

#include <string>
#include <windows.h>

#include "logger.h"

class BasicCommand
{
public:
    explicit BasicCommand(Logger *logger);
    virtual ~BasicCommand();

    virtual void execute() = 0;

protected:
    Logger *logger_;

    bool executeBlockingCommand(const wchar_t *cmd, std::string &answer);

private:
    std::string readAllFromPipe(HANDLE hPipe);
};
