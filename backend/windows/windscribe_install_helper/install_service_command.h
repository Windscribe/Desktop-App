#pragma once
#include "basic_command.h"

class InstallServiceCommand :
	public BasicCommand
{
public:
	InstallServiceCommand(Logger *logger, const wchar_t *servicePath, const wchar_t *subinaclPath);
	~InstallServiceCommand() override;

	void execute() override;

private:
	std::wstring servicePath_;
	std::wstring subinaclPath_;

	bool serviceExists(const wchar_t *serviceName);
	void simpleStopService(const wchar_t *serviceName);
	bool waitForService(SC_HANDLE serviceHandle, DWORD status);
	void simpleDeleteService(const wchar_t *serviceName);
	void fixUnquotedServicePath();
};

