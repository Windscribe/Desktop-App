#include "install_service_command.h"


InstallServiceCommand::InstallServiceCommand(Logger *logger, const wchar_t *servicePath, const wchar_t *subinaclPath) :
	BasicCommand(logger), servicePath_(servicePath), subinaclPath_(subinaclPath)
{
}


InstallServiceCommand::~InstallServiceCommand()
{
}

void InstallServiceCommand::execute()
{
	if (serviceExists(L"WindscribeService"))
	{
		logger_->outStr("Remove previous instance of WindscribeService\n");
		simpleStopService(L"WindscribeService");
		simpleDeleteService(L"WindscribeService");
	}

	std::wstring cmd;
	std::string answer;
	cmd = L"sc create WindscribeService binPath= \"";
	cmd += servicePath_ + L"\" start= auto";
	if (executeBlockingCommand(cmd.c_str(), answer))
	{
		logger_->outStr("Output from command (sc create WindscribeService):\n");
		logger_->outStr(answer.c_str());
	}
	else
	{
		logger_->outStr("Failed execute command (sc create WindscribeService)\n");
		return;
	}

	cmd = L"sc description WindscribeService \"Manages the firewall and controls the VPN tunnel\"";
	if (executeBlockingCommand(cmd.c_str(), answer))
	{
		logger_->outStr("Output from command (sc description WindscribeService):\n");
		logger_->outStr(answer.c_str());
	}
	else
	{
		logger_->outStr("Failed execute command (sc description WindscribeService)\n");
		return;
	}

	cmd = L"\"" + subinaclPath_ + L"\"";
	cmd += L" /SERVICE WindscribeService /grant=S-1-5-11=F"; 
	if (executeBlockingCommand(cmd.c_str(), answer))
	{
		logger_->outStr("Subinacl command executed\n");
		fixUnquotedServicePath();
	}
	else
	{
		logger_->outStr("Failed execute command subinacl\n");
		return;
	}
}

bool InstallServiceCommand::serviceExists(const wchar_t *serviceName)
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		return false;
	}

	SC_HANDLE serviceHandle = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS);
	if (!serviceHandle)
	{
		CloseServiceHandle(schSCManager);
		return false;
	}

	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(schSCManager);
	return true;
}

void InstallServiceCommand::simpleStopService(const wchar_t *serviceName)
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		return;
	}

	SC_HANDLE serviceHandle = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS);
	if (!serviceHandle)
	{
		CloseServiceHandle(schSCManager);
		return;
	}
	SERVICE_STATUS serviceStatus;
	ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);
	waitForService(serviceHandle, SERVICE_STOPPED);

	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(schSCManager);
}

bool InstallServiceCommand::waitForService(SC_HANDLE serviceHandle, DWORD status)
{
	DWORD pendingStatus;
	if (status == SERVICE_RUNNING)
	{
		pendingStatus = SERVICE_START_PENDING;
	}
	else if (status == SERVICE_STOPPED)
	{
		pendingStatus = SERVICE_STOP_PENDING;
	}

	while (true)
	{
		SERVICE_STATUS serviceStatus;
		if (!ControlService(serviceHandle, SERVICE_CONTROL_INTERROGATE, &serviceStatus))
		{
			return false;
		}
		if (serviceStatus.dwWin32ExitCode != 0)
		{
			return false;
		}
		bool result = (serviceStatus.dwCurrentState == status);
		if (!result && serviceStatus.dwCurrentState == pendingStatus)
		{
			Sleep(serviceStatus.dwWaitHint);
		}
		else
		{
			break;
		}
	}

	return true;
}

void InstallServiceCommand::simpleDeleteService(const wchar_t *serviceName)
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		return;
	}

	SC_HANDLE serviceHandle = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS);
	if (!serviceHandle)
	{
		CloseServiceHandle(schSCManager);
		return;
	}
	DeleteService(serviceHandle);
	
	CloseServiceHandle(serviceHandle);
	CloseServiceHandle(schSCManager);
}

void InstallServiceCommand::fixUnquotedServicePath()
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\WindscribeService", 0, KEY_READ | KEY_WRITE, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		WCHAR szBuffer[MAX_PATH];
		DWORD dwBufferSize = sizeof(szBuffer);
		DWORD dwType;
		if (RegQueryValueEx(hKey, L"ImagePath", 0, &dwType, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS)
		{
			std::wstring ws(szBuffer);
			if (ws.length() > 0 && ws[0] != L'"')
			{
				ws.insert(0, 1, L'"');
				ws.insert(ws.length(), 1, L'"');
				RegSetValueEx(hKey, L"ImagePath", 0, dwType, (BYTE *)ws.c_str(), ws.size() * sizeof(wchar_t));
			}
		}
		RegCloseKey(hKey);
	}
}