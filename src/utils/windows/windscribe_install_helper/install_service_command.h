#pragma once

#include "basic_command.h"

class InstallServiceCommand : public BasicCommand
{
public:
    InstallServiceCommand(Logger *logger, const std::wstring &installDir);
    ~InstallServiceCommand() override;

    void execute() override;

private:
    const std::wstring installDir_;
};
