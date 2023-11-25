#pragma once
#include "basic_command.h"

class InstallServiceCommand : public BasicCommand
{
public:
    InstallServiceCommand(Logger *logger, const wchar_t *servicePath);
    ~InstallServiceCommand() override;

    void execute() override;

private:
    std::wstring servicePath_;
};
