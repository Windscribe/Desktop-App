#pragma once

#include <vector>
#include <string>

namespace IPC
{

// base class for all commands
class Command
{
public:
    virtual ~Command() {}

    virtual std::vector<char> getData() const = 0;

    // return unique static string ID for command
    virtual std::string getStringId() const = 0;

    // return debug string of command
    virtual std::string getDebugString() const = 0;
};

} // namespace IPC
