#pragma once

#include "command.h"

namespace IPC
{

static const int CLIENT_CMD_ClientAuth = 0;

class CommandFactory
{
public:
    static Command *makeCommand(const std::string strId, char *buf, int size);
};

} // namespace IPC
