#pragma once

#include "basic_command.h"

class CommandParser
{
public:
    CommandParser();
    virtual ~CommandParser();

    BasicCommand *parse(int argc, wchar_t* argv[]);
};
