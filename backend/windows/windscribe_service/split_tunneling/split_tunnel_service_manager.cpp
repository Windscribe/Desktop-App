#include "../all_headers.h"
#include "split_tunnel_service_manager.h"
#include "../logger.h"

SplitTunnelServiceManager::SplitTunnelServiceManager()
{
	bStarted_ = false;
}

void SplitTunnelServiceManager::start()
{
	Logger::instance().out(L"SplitTunnelServiceManager::start()");
	
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!schSCManager)
	{
		Logger::instance().out(L"SplitTunnelServiceManager::start(), OpenSCManager failed with error: %ld", GetLastError());
		goto end_label;
	}

	schService = OpenService(schSCManager, L"WindscribeSplitTunnel", SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (schService == NULL)
	{
		Logger::instance().out(L"SplitTunnelServiceManager::start(), OpenService failed with error: %ld", GetLastError());
		goto end_label;
	}

	if (StartService(schService, NULL, NULL) == FALSE)
	{
		DWORD dwErr = GetLastError();
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
			bStarted_ = true;
			goto end_label;
		}
		else
		{
			Logger::instance().out(L"SplitTunnelServiceManager::start(), StartService failed with error: %ld", dwErr);
			goto end_label;
		}
	}

	bStarted_ = true;

end_label:

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
	}
}

void SplitTunnelServiceManager::stop()
{
	Logger::instance().out(L"SplitTunnelServiceManager::stop()");
	
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!schSCManager)
	{
		Logger::instance().out(L"SplitTunnelServiceManager::start(), OpenSCManager failed with error: %ld", GetLastError());
		goto end_label;
	}

	schService = OpenService(schSCManager, L"WindscribeSplitTunnel", SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (schService == NULL)
	{
		Logger::instance().out(L"SplitTunnelServiceManager::start(), OpenService failed with error: %ld", GetLastError());
		goto end_label;
	}

	SERVICE_STATUS ss;
	if (!ControlService(schService, SERVICE_CONTROL_STOP, &ss))
	{
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
		{
			bStarted_ = false;
		}
		else
		{
			Logger::instance().out(L"SplitTunnelServiceManager::start(), StartService failed with error: %ld", dwErr);
			goto end_label;
		}
	}

	bStarted_ = false;

end_label:

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
	}
}