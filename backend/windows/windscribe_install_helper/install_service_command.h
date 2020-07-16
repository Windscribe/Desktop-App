#pragma once
#include "basic_command.h"

class InstallServiceCommand :
	public BasicCommand
{
public:
	InstallServiceCommand(Logger *logger, const wchar_t *servicePath, const wchar_t *subinaclPath);
	virtual ~InstallServiceCommand();

	virtual void execute();

private:
	std::wstring servicePath_;
	std::wstring subinaclPath_;

	bool serviceExists(const wchar_t *serviceName);
	void simpleStopService(const wchar_t *serviceName);
	bool waitForService(SC_HANDLE serviceHandle, DWORD status);
	void simpleDeleteService(const wchar_t *serviceName);
	void fixUnquotedServicePath();
};

