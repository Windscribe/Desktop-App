#include "all_headers.h"
#include "split_tunneling/split_tunneling.h"
#include "ipv6_firewall.h"
#include "dns_firewall.h"
#include "firewallfilter.h"
#include "logger.h"
#include "ipc/servicecommunication.h"
#include "icsmanager.h"
#include "sys_ipv6_controller.h"
#include "hostsedit.h"
#include "get_active_processes.h"
#include "pipe_for_process.h"
#include "executecmd.h"
#include "reinstall_wan_ikev2.h"
#include "close_tcp_connections.h"
#include "cleardns_on_tap.h"
#include "utils.h"
#include "registry.h"
#include "ikev2ipsec.h"
#include "ikev2route.h"
#include "ioutils.h"
#include "ipc/serialize_structs.h"
#include "fwpm_wrapper.h"
#include "remove_windscribe_network_profiles.h"
#include "wireguard/defaultroutemonitor.h"
#include "wireguard/wireguardadapter.h"
#include "wireguard/wireguardcontroller.h"
#include <conio.h>
#include "../../../common/utils/executable_signature/executable_signature_win.h"
#include "../../../common/utils/crashhandler.h"

#define SERVICE_NAME  (L"WindscribeService")
#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE				  g_hThread = NULL;
//HostsEdit             g_hostsEdit;

bool bOn = false;
bool bOff = false;

VOID WINAPI serviceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI serviceCtrlHandler(DWORD);
DWORD WINAPI serviceWorkerThread(LPVOID lpParam);
BOOL isElevated();

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		if (strcmp(argv[1], "-firewall_off") == 0)
		{
			if (isElevated())
			{
				FwpmWrapper	  fwpmHandleWrapper;

				Ipv6Firewall ipv6firewall(fwpmHandleWrapper);
				ipv6firewall.enableIPv6();

				DnsFirewall dnsFirewall(fwpmHandleWrapper);
				dnsFirewall.disable();

				FirewallFilter firewallFilter(fwpmHandleWrapper);
				firewallFilter.off();

				SplitTunneling::removeAllFilters(fwpmHandleWrapper);
				printf("Windscribe firewall deleted.\n");
			}
			else
			{
				printf("Need run program with admin rights.\n");
			}
			return 0;
		}
	}

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"service");
    Debug::CrashHandler::instance().bindToProcess();
#endif

#ifdef _DEBUG
	// for debug (run without service)
	g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	HANDLE hThread = CreateThread (NULL, 0, serviceWorkerThread, NULL, 0, NULL);
	_getch();
	SetEvent (g_ServiceStopEvent);
	WaitForSingleObject (hThread, INFINITE);
    CloseHandle(hThread);
	CloseHandle (g_ServiceStopEvent);
#else
		
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ (LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)serviceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(serviceTable) == FALSE)
	{
		return GetLastError();
	}
#endif

	return 0;
}

BOOL isElevated() 
{
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
	{
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) 
		{
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) 
	{
		CloseHandle(hToken);
	}
	return fRet;
}

VOID WINAPI serviceMain(DWORD, LPTSTR *)
{
	// Register our service control handler with the SCM
	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, serviceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T(
		//  "My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	//
	// Perform tasks necessary to start the service here
	//

	// Create a service stop event to wait on later
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		// Error creating event
		// Tell service controller we are stopped and exit
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//OutputDebugString(_T(
			//"My Sample Service: ServiceMain: SetServiceStatus returned error"));
		}
		goto EXIT;
	}

	// Start a thread that will perform the main task of the service
	g_hThread = CreateThread(NULL, 0, serviceWorkerThread, NULL, 0, NULL);

	// Wait until our worker thread exits signaling that the service needs to stop
	WaitForSingleObject(g_hThread, INFINITE);


	//
	// Perform any cleanup tasks
	//

	CloseHandle(g_ServiceStopEvent);

	// Tell the service controller we are stopped
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T(
		//"My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	CloseHandle(g_hThread);
	g_hThread = NULL;

EXIT:
	return;
}

VOID WINAPI serviceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		Logger::instance().out(L"SERVICE_CONTROL_STOP message received");

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		//
		// Perform tasks necessary to stop the service here
		//

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//OutputDebugString(_T(
			//"My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	case SERVICE_CONTROL_SHUTDOWN:
		Logger::instance().out(L"SERVICE_CONTROL_SHUTDOWN message received");
		SetEvent(g_ServiceStopEvent);
		WaitForSingleObject(g_hThread, INFINITE);
		CloseHandle(g_hThread);
		g_hThread = NULL;
		break;

	default:
		break;
	}
}

BOOL CreateDACL(SECURITY_ATTRIBUTES *pSA)
{
	TCHAR * szSD = TEXT("D:")         // Discretionary ACL
		TEXT("(D;OICI;GA;;;AN)")      // Deny access to anonymous logon
		TEXT("(A;OICI;GRGWGX;;;BG)")  // Allow access to built-in guests
		TEXT("(A;OICI;GRGWGX;;;AU)")  // Allow read/write/execute to authenticated  users
		TEXT("(A;OICI;GA;;;BA)");     // Allow full control to administrators

	if (NULL == pSA)
		return FALSE;

	return ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &(pSA->lpSecurityDescriptor),	NULL);
}

HANDLE CreatePipe()
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;

	if (CreateDACL(&sa))
	{

		HANDLE hPipe = ::CreateNamedPipe(SERVICE_PIPE_NAME,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			1, 4096,
			4096, NMPWAIT_USE_DEFAULT_WAIT, &sa);
		return hPipe;
	}
	else
	{
		return INVALID_HANDLE_VALUE;
	}
}

std::string getHelperVersion()
{
	wchar_t szModPath[MAX_PATH];
	szModPath[0] = '\0';
	GetModuleFileName(NULL, szModPath, sizeof(szModPath));

	DWORD dwArg;
	DWORD dwInfoSize = GetFileVersionInfoSize(szModPath, &dwArg);
	if (dwInfoSize == 0)
	{
		return "Failed";
	}

	std::unique_ptr<unsigned char> arr(new unsigned char[dwInfoSize]);
	if (GetFileVersionInfo(szModPath, 0, dwInfoSize, arr.get()) == 0)
	{
		return "Failed";
	}
	VS_FIXEDFILEINFO *vInfo;

	UINT uInfoSize;
	if (VerQueryValue(arr.get(), TEXT("\\"), (LPVOID*)&vInfo, &uInfoSize) == 0)
	{
		return "Failed";
	}
	WORD v1 = HIWORD(vInfo->dwFileVersionMS);
	WORD v2 = LOWORD(vInfo->dwFileVersionMS);
	WORD v3 = HIWORD(vInfo->dwFileVersionLS);
	WORD v4 = LOWORD(vInfo->dwFileVersionLS);
	std::string s = std::to_string(v1) + "." + std::to_string(v2) + "." + std::to_string(v3) + "." + std::to_string(v4);
	return s;
}

MessagePacketResult processMessagePacket(int cmdId, const std::string &packet, IcsManager &icsManager, FirewallFilter &firewallFilter, Ipv6Firewall &ipv6Firewall, DnsFirewall &dnsFirewall,
										 SysIpv6Controller &sysIpv6Controller, HostsEdit &hostsEdit, GetActiveProcesses &getActiveProcesses,
										 SplitTunneling &splitTunnelling, WireGuardController &wireGuardController)
{
	MessagePacketResult mpr;
	mpr.success = false;

	std::istringstream  stream(packet);
	boost::archive::text_iarchive ia(stream, boost::archive::no_header);

	if (cmdId == AA_COMMAND_FIREWALL_ON)
	{
		CMD_FIREWALL_ON cmdFirewallOn;
		ia >> cmdFirewallOn;

		firewallFilter.on(cmdFirewallOn.ip.c_str(), cmdFirewallOn.allowLanTraffic);
		Logger::instance().out(L"AA_COMMAND_FIREWALL_ON, AllowLocalTraffic=%d", cmdFirewallOn.allowLanTraffic);
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_FIREWALL_OFF)
	{
		firewallFilter.off();
		Logger::instance().out(L"AA_COMMAND_FIREWALL_OFF");
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_FIREWALL_CHANGE)
	{
		CMD_FIREWALL_ON cmdFirewallOn;
		ia >> cmdFirewallOn;

		firewallFilter.on(cmdFirewallOn.ip.c_str(), cmdFirewallOn.allowLanTraffic);
		Logger::instance().out(L"AA_COMMAND_FIREWALL_CHANGE, AllowLocalTraffic=%d", cmdFirewallOn.allowLanTraffic);
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_FIREWALL_STATUS)
	{
		mpr.success = true;
		if (firewallFilter.currentStatus())
		{
			Logger::instance().out(L"AA_COMMAND_FIREWALL_STATUS, On");
			mpr.exitCode = 1;
		}
		else
		{
			Logger::instance().out(L"AA_COMMAND_FIREWALL_STATUS, Off");
			mpr.exitCode = 0;
		}
	}
	else if (cmdId == AA_COMMAND_FIREWALL_IPV6_ENABLE)
	{
		Logger::instance().out(L"AA_COMMAND_FIREWALL_IPV6_ENABLE");
		ipv6Firewall.enableIPv6();
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_DISABLE_DNS_TRAFFIC)
	{
		Logger::instance().out(L"AA_COMMAND_DISABLE_DNS_TRAFFIC");
		dnsFirewall.enable();
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_ENABLE_DNS_TRAFFIC)
	{
		Logger::instance().out(L"AA_COMMAND_ENABLE_DNS_TRAFFIC");
		dnsFirewall.disable();
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_FIREWALL_IPV6_DISABLE)
	{
		Logger::instance().out(L"AA_COMMAND_FIREWALL_IPV6_DISABLE");
		ipv6Firewall.disableIPv6();
		mpr.success = true;
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS)
	{
		Logger::instance().out(L"AA_COMMAND_REMOVE_HOSTS");
		mpr.success = hostsEdit.removeWindscribeHosts();
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_ADD_HOSTS)
	{
		Logger::instance().out(L"AA_COMMAND_ADD_HOSTS");
		CMD_ADD_HOSTS cmdAddHosts;
		ia >> cmdAddHosts;
		mpr.success = hostsEdit.addHosts(cmdAddHosts.hosts);
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_REMOVE_HOSTS)
	{
		Logger::instance().out(L"AA_COMMAND_REMOVE_HOSTS");
		mpr.success = hostsEdit.removeHosts();
		mpr.exitCode = 0;
	}
	else if (cmdId == AA_COMMAND_CHECK_UNBLOCKING_CMD_STATUS)
	{
		CMD_CHECK_UNBLOCKING_CMD_STATUS checkUnblockingCmdStatus;
		ia >> checkUnblockingCmdStatus;
		mpr = ExecuteCmd::instance().getUnblockingCmdStatus(checkUnblockingCmdStatus.cmdId);
	}
	else if (cmdId == AA_COMMAND_GET_UNBLOCKING_CMD_COUNT)
	{
		mpr = ExecuteCmd::instance().getActiveUnblockingCmdCount();
	}
	else if (cmdId == AA_COMMAND_CLEAR_UNBLOCKING_CMD)
	{
		CMD_CLEAR_UNBLOCKING_CMD cmdClearUnblockingCmd;
		ia >> cmdClearUnblockingCmd;
		mpr = ExecuteCmd::instance().clearUnblockingCmd(cmdClearUnblockingCmd.blockingCmdId);
	}
	else if (cmdId == AA_COMMAND_SUSPEND_UNBLOCKING_CMD)
	{
		CMD_SUSPEND_UNBLOCKING_CMD cmdSuspendUnblockingCmd;
		ia >> cmdSuspendUnblockingCmd;
		mpr = ExecuteCmd::instance().suspendUnblockingCmd(cmdSuspendUnblockingCmd.blockingCmdId);
	}
	else if (cmdId == AA_COMMAND_GET_HELPER_VERSION)
	{
		mpr.success = true;
		mpr.additionalString = getHelperVersion();
	}
	else if (cmdId == AA_COMMAND_OS_IPV6_STATE)
	{
		mpr.exitCode = sysIpv6Controller.isIpv6Enabled();
		Logger::instance().out(L"AA_COMMAND_OS_IPV6_STATE, %d", mpr.exitCode);
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_OS_IPV6_ENABLE)
	{
		Logger::instance().out(L"AA_COMMAND_OS_IPV6_ENABLE");
		sysIpv6Controller.setIpv6Enabled(true);
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_OS_IPV6_DISABLE)
	{
		Logger::instance().out(L"AA_COMMAND_OS_IPV6_DISABLE");
		sysIpv6Controller.setIpv6Enabled(false);
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_IS_SUPPORTED_ICS)
	{
		Logger::instance().out(L"AA_COMMAND_IS_SUPPORTED_ICS");
		mpr.exitCode = icsManager.isSupported();
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_REINSTALL_WAN_IKEV2)
	{
		Logger::instance().out(L"AA_COMMAND_REINSTALL_WAN_IKEV2");
		bool b1 = ReinstallWanIkev2::uninstallDevice();
		bool b2 = ReinstallWanIkev2::scanForHardwareChanges();
		mpr.exitCode = b1 && b2;
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_ENABLE_WAN_IKEV2)
	{
		Logger::instance().out(L"AA_COMMAND_ENABLE_WAN_IKEV2");
		mpr.exitCode = ReinstallWanIkev2::enableDevice();
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_CLOSE_TCP_CONNECTIONS)
	{
        CMD_CLOSE_TCP_CONNECTIONS cmdCloseTcpConnections;
        ia >> cmdCloseTcpConnections;
        Logger::instance().out(L"AA_COMMAND_CLOSE_TCP_CONNECTIONS, KeepLocalSockets = %d",
                               cmdCloseTcpConnections.isKeepLocalSockets);
		CloseTcpConnections::closeAllTcpConnections(cmdCloseTcpConnections.isKeepLocalSockets);
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_ENUM_PROCESSES)
	{
		Logger::instance().out(L"AA_COMMAND_ENUM_PROCESSES");
		
		std::vector<std::wstring> list = getActiveProcesses.getList();

		size_t overallCharactersCount = 0;
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			overallCharactersCount += it->length();
			overallCharactersCount += 1;  // add null character after each process name
		}

		if (overallCharactersCount > 0)
		{
			std::vector<char> v(overallCharactersCount * sizeof(wchar_t));
			size_t curPos = 0;
			wchar_t *p = (wchar_t *)(&v[0]);
			for (auto it = list.begin(); it != list.end(); ++it)
			{
				memcpy(p + curPos, it->c_str(), it->length() * sizeof(wchar_t));
				curPos += it->length();
				p[curPos] = L'\0';
				curPos++;
			}
			mpr.additionalString = std::string(v.begin(), v.end());
		}
		
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_TASK_KILL)
	{
		CMD_TASK_KILL cmdTaskKill;
		ia >> cmdTaskKill;

		wchar_t killCmd[MAX_PATH];
		wcscpy(killCmd, L"taskkill /f /im ");
		wcscat(killCmd, cmdTaskKill.szExecutableName.c_str());
		Logger::instance().out(L"AA_COMMAND_TASK_KILL, cmd=%s", killCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(killCmd);
	}
	else if (cmdId == AA_COMMAND_RESET_TAP)
	{
		CMD_RESET_TAP cmdResetTap;
		ia >> cmdResetTap;

		wchar_t resetCmd[MAX_PATH];
		wcscpy(resetCmd, L"wmic path win32_networkadapter where Description=\"");
		wcscat(resetCmd, cmdResetTap.szTapName.c_str());
		wcscat(resetCmd, L"\" call disable");
		Logger::instance().out(L"AA_COMMAND_RESET_TAP, cmd1=%s", resetCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(resetCmd);

		wcscpy(resetCmd, L"wmic path win32_networkadapter where Description=\"");
		wcscat(resetCmd, cmdResetTap.szTapName.c_str());
		wcscat(resetCmd, L"\" call enable");
		Logger::instance().out(L"AA_COMMAND_RESET_TAP, cmd2=%s", resetCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(resetCmd);
	}
	else if (cmdId == AA_COMMAND_SET_METRIC)
	{
		CMD_SET_METRIC cmdSetMetric;
		ia >> cmdSetMetric;

		wchar_t setMetricCmd[MAX_PATH];
		wcscpy(setMetricCmd, L"netsh int ");
		wcscat(setMetricCmd, cmdSetMetric.szInterfaceType.c_str());
		wcscat(setMetricCmd, L" set interface interface=\"");
		wcscat(setMetricCmd, cmdSetMetric.szInterfaceName.c_str());
		wcscat(setMetricCmd, L"\" metric=");
		wcscat(setMetricCmd, cmdSetMetric.szMetricNumber.c_str());
		
		Logger::instance().out(L"AA_COMMAND_SET_METRIC, cmd=%s", setMetricCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(setMetricCmd);
	}
	else if (cmdId == AA_COMMAND_WMIC_ENABLE)
	{
		CMD_WMIC_ENABLE cmdWmicEnable;
		ia >> cmdWmicEnable;

		wchar_t wmicCmd[MAX_PATH];
		wcscpy(wmicCmd, L"wmic path win32_networkadapter where description=\"");
		wcscat(wmicCmd, cmdWmicEnable.szAdapterName.c_str());
		wcscat(wmicCmd, L"\" call enable");
		
		Logger::instance().out(L"AA_COMMAND_WMIC_ENABLE, cmd=%s", wmicCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(wmicCmd);
	}
	else if (cmdId == AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE)
	{
		CMD_WMIC_GET_CONFIG_ERROR_CODE cmdWmicGetConfigErrorCode;
		ia >> cmdWmicGetConfigErrorCode;

		wchar_t wmicCmd[MAX_PATH];
		wcscpy(wmicCmd, L"wmic path win32_networkadapter where description=\"");
		wcscat(wmicCmd, cmdWmicGetConfigErrorCode.szAdapterName.c_str());
		wcscat(wmicCmd, L"\" get ConfigManagerErrorCode");

		Logger::instance().out(L"AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE, cmd=%s", wmicCmd);
		mpr = ExecuteCmd::instance().executeBlockingCmd(wmicCmd);
	}
	else if (cmdId == AA_COMMAND_CLEAR_DNS_ON_TAP)
	{
		Logger::instance().out(L"AA_COMMAND_CLEAR_DNS_ON_TAP");
		ClearDnsOnTap::clearDns();
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_ENABLE_BFE)
	{
		Logger::instance().out(L"AA_COMMAND_ENABLE_BFE");

		wchar_t bfeCmd[MAX_PATH];
		wcscpy(bfeCmd, L"sc config BFE start= auto");
		mpr = ExecuteCmd::instance().executeBlockingCmd(bfeCmd);
		wcscpy(bfeCmd, L"sc start BFE");
		mpr = ExecuteCmd::instance().executeBlockingCmd(bfeCmd);
	}
	else if (cmdId == AA_COMMAND_RESET_AND_START_RAS)
	{
		Logger::instance().out(L"AA_COMMAND_RESET_AND_START_RAS");

		wchar_t serviceCmd[MAX_PATH];
		wcscpy(serviceCmd, L"sc config RasMan start= demand");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);
		wcscpy(serviceCmd, L"sc stop RasMan");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);
		wcscpy(serviceCmd, L"sc start RasMan");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);

		wcscpy(serviceCmd, L"sc config SstpSvc start= demand");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);
		wcscpy(serviceCmd, L"sc stop SstpSvc");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);
		wcscpy(serviceCmd, L"sc start SstpSvc");
		mpr = ExecuteCmd::instance().executeBlockingCmd(serviceCmd);
	}
	else if (cmdId == AA_COMMAND_RUN_OPENVPN)
	{
		CMD_RUN_OPENVPN cmdRunOpenVpn;
		ia >> cmdRunOpenVpn;

		// check input parameters
		if (Utils::isValidFileName(cmdRunOpenVpn.szOpenVpnExecutable) &&
			Utils::isFileExists(cmdRunOpenVpn.szConfigPath.c_str()) &&
			Utils::noSpacesInString(cmdRunOpenVpn.szHttpProxy) &&
			Utils::noSpacesInString(cmdRunOpenVpn.szSocksProxy))
		{
			// make openvpn command
			std::wstring strCmd = L"\"" + Utils::getExePath() + L"\\" + cmdRunOpenVpn.szOpenVpnExecutable + L"\"";
			strCmd += L" --config \"" + cmdRunOpenVpn.szConfigPath + L"\" --management 127.0.0.1 ";
			strCmd += std::to_wstring(cmdRunOpenVpn.portNumber) + L" --management-query-passwords --management-hold";

			if (wcslen(cmdRunOpenVpn.szHttpProxy.c_str()) > 0)
			{
				strCmd += L" --http-proxy " + cmdRunOpenVpn.szHttpProxy + L" " + std::to_wstring(cmdRunOpenVpn.httpPortNumber) + L" auto";
			}
			if (wcslen(cmdRunOpenVpn.szSocksProxy.c_str()) > 0)
			{
				strCmd += L" --socks-proxy " + cmdRunOpenVpn.szSocksProxy + L" " + std::to_wstring(cmdRunOpenVpn.socksPortNumber);
			}

			Logger::instance().out(L"AA_COMMAND_RUN_OPENVPN, cmd=%s", strCmd.c_str());

			wchar_t szWorkingDir[MAX_PATH];
			std::wstring strConfigPath = cmdRunOpenVpn.szConfigPath;
			wcscpy(szWorkingDir, Utils::getDirPathFromFullPath(strConfigPath).c_str());
			Logger::instance().out(L"AA_COMMAND_RUN_OPENVPN, ovpn file directory=%s", szWorkingDir);

			mpr = ExecuteCmd::instance().executeUnblockingCmd(strCmd.c_str(), L"", szWorkingDir);
		}
		else
		{
			mpr.success = false;
		}
	}
	else if (cmdId == AA_COMMAND_UPDATE_ICS)
	{
		CMD_UPDATE_ICS cmdUpdateIcs;
		ia >> cmdUpdateIcs;

		// make command line
		std::wstring strCmd = L"\"" + Utils::getExePath() + L"\\ChangeIcs.exe\"";
		if (cmdUpdateIcs.cmd == 0)  // save
		{
			strCmd += L" -save \"";
			strCmd += cmdUpdateIcs.szConfigPath;
			strCmd += L"\"";
			mpr = ExecuteCmd::instance().executeUnblockingCmd(strCmd.c_str(), cmdUpdateIcs.szEventName.c_str(), NULL);
			Logger::instance().out(L"AA_COMMAND_UPDATE_ICS, cmd=%s", strCmd.c_str());
		}
		else if (cmdUpdateIcs.cmd == 1) // restore
		{
			strCmd += L" -restore \"";
			strCmd += cmdUpdateIcs.szConfigPath;
			strCmd += L"\"";
			mpr = ExecuteCmd::instance().executeUnblockingCmd(strCmd.c_str(), cmdUpdateIcs.szEventName.c_str(), NULL);
			Logger::instance().out(L"AA_COMMAND_UPDATE_ICS, cmd=%s", strCmd.c_str());
		}
		else if (cmdUpdateIcs.cmd == 2) // change
		{
			strCmd += L" -change ";
			strCmd += cmdUpdateIcs.szPublicGuid;
			strCmd += L" ";
			strCmd += cmdUpdateIcs.szPrivateGuid;
			mpr = ExecuteCmd::instance().executeUnblockingCmd(strCmd.c_str(), cmdUpdateIcs.szEventName.c_str(), NULL);
			Logger::instance().out(L"AA_COMMAND_UPDATE_ICS, cmd=%s", strCmd.c_str());
		}
		else
		{
			mpr.success = false;
		}
	}
	else if (cmdId == AA_COMMAND_WHITELIST_PORTS)
	{
		CMD_WHITELIST_PORTS cmdWhitelistPorts;
		ia >> cmdWhitelistPorts;

		wchar_t szBuf[1024];
		std::wstring strCmd = L"netsh advfirewall firewall add rule name=\"WindscribeStaticIpTcp\" protocol=TCP dir=in localport=\"";
		strCmd += cmdWhitelistPorts.ports;
		strCmd += L"\" action=allow";
		wcscpy(szBuf, strCmd.c_str());
		mpr = ExecuteCmd::instance().executeBlockingCmd(szBuf);
		Logger::instance().out(L"AA_COMMAND_WHITELIST_PORTS, cmd=%s", strCmd.c_str());

		strCmd = L"netsh advfirewall firewall add rule name=\"WindscribeStaticIpUdp\" protocol=UDP dir=in localport=\"";
		strCmd += cmdWhitelistPorts.ports;
		strCmd += L"\" action=allow";
		wcscpy(szBuf, strCmd.c_str());
		mpr = ExecuteCmd::instance().executeBlockingCmd(szBuf);
		Logger::instance().out(L"AA_COMMAND_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
	}
	else if (cmdId == AA_COMMAND_DELETE_WHITELIST_PORTS)
	{
		wchar_t szBuf[1024];
		std::wstring strCmd = L"netsh advfirewall firewall delete rule name=\"WindscribeStaticIpTcp\" dir=in";
		wcscpy(szBuf, strCmd.c_str());
		mpr = ExecuteCmd::instance().executeBlockingCmd(szBuf);
		Logger::instance().out(L"AA_COMMAND_DELETE_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
		strCmd = L"netsh advfirewall firewall delete rule name=\"WindscribeStaticIpUdp\" dir=in";
		wcscpy(szBuf, strCmd.c_str());
		mpr = ExecuteCmd::instance().executeBlockingCmd(szBuf);
		Logger::instance().out(L"AA_COMMAND_DELETE_WHITELIST_PORTS, cmd=%s", strCmd.c_str());
	}
	else if (cmdId == AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ)
	{
		CMD_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ cmdSetMacAddressRegistryValueSz;
		ia >> cmdSetMacAddressRegistryValueSz;

		std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" + cmdSetMacAddressRegistryValueSz.szInterfaceName;
		std::wstring propertyName = L"NetworkAddress";
		std::wstring propertyValue = cmdSetMacAddressRegistryValueSz.szValue;

		mpr.success = Registry::regWriteSzProperty(HKEY_LOCAL_MACHINE, keyPath.c_str(), propertyName, propertyValue);

		if (mpr.success)
		{
			Registry::regWriteDwordProperty(HKEY_LOCAL_MACHINE, keyPath, L"WindscribeMACSpoofed", 1);
		}

		Logger::instance().out(L"AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ, path=%s, name=%s, value=%s", keyPath.c_str(), propertyName.c_str(), propertyValue.c_str());

	}
	else if (cmdId == AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY)
	{
		CMD_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY cmdRemoveMacAddressRegistryProperty;
		ia >> cmdRemoveMacAddressRegistryProperty;

		std::wstring propertyName = L"NetworkAddress";
		std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" + cmdRemoveMacAddressRegistryProperty.szInterfaceName;

		wchar_t keyPathSz[128];
		wcscpy(keyPathSz, keyPath.c_str());

		bool success1 = Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, keyPathSz, propertyName);
		bool success2 = false;

		if (success1)
		{
			success2 = Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, keyPathSz, L"WindscribeMACSpoofed");
		}

		mpr.success = success1 && success2;

		Logger::instance().out(L"AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY, key=%s, name=%s", keyPathSz, propertyName.c_str());
	}
	else if (cmdId == AA_COMMAND_RESET_NETWORK_ADAPTER)
	{
		CMD_RESET_NETWORK_ADAPTER cmdResetNetworkAdapter;
		ia >> cmdResetNetworkAdapter;

		std::wstring adapterRegistryName = cmdResetNetworkAdapter.szInterfaceName;

		Logger::instance().out(L"AA_COMMAND_RESET_NETWORK_ADAPTER, Disable %s", adapterRegistryName.c_str());
		Utils::callNetworkAdapterMethod(L"Disable", adapterRegistryName);

		if (cmdResetNetworkAdapter.bringBackUp)
		{
			Logger::instance().out(L"AA_COMMAND_RESET_NETWORK_ADAPTER, Enable %s", adapterRegistryName.c_str());
			Utils::callNetworkAdapterMethod(L"Enable", adapterRegistryName);
		}

		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_SPLIT_TUNNELING_SETTINGS)
	{
		CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
		ia >> cmdSplitTunnelingSettings;

		Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS, isEnabled: %d, isExclude: %d,"
                               L" isKeepLocalSockets: %d, cntApps: %d",
                               cmdSplitTunnelingSettings.isActive,
                               cmdSplitTunnelingSettings.isExclude,
                               cmdSplitTunnelingSettings.isKeepLocalSockets,
                               cmdSplitTunnelingSettings.files.size());

		for (size_t i = 0; i < cmdSplitTunnelingSettings.files.size(); ++i)
		{
			Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s", cmdSplitTunnelingSettings.files[i].c_str());
		}

		for (size_t i = 0; i < cmdSplitTunnelingSettings.ips.size(); ++i)
		{
			Logger::instance().out(L"AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s", cmdSplitTunnelingSettings.ips[i].c_str());
		}

		for (size_t i = 0; i < cmdSplitTunnelingSettings.hosts.size(); ++i)
		{
			Logger::instance().out("AA_COMMAND_SPLIT_TUNNELING_SETTINGS: %s", cmdSplitTunnelingSettings.hosts[i].c_str());
		}

        splitTunnelling.setKeepLocalSocketsOnDisconnect(
            cmdSplitTunnelingSettings.isKeepLocalSockets);

		if (cmdSplitTunnelingSettings.isActive)
		{
			splitTunnelling.start();
			splitTunnelling.setSettings(cmdSplitTunnelingSettings.isExclude, cmdSplitTunnelingSettings.files, cmdSplitTunnelingSettings.ips, cmdSplitTunnelingSettings.hosts);
		}
		else
		{
			splitTunnelling.stop();
		}
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_CONNECT_STATUS)
	{
		CMD_CONNECT_STATUS cmdConnectStatus;
		ia >> cmdConnectStatus;
		Logger::instance().out(L"AA_COMMAND_CONNECT_STATUS: %d", cmdConnectStatus.isConnected);
		splitTunnelling.setConnectStatus(cmdConnectStatus.isConnected);
	}
	else if (cmdId == AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE)
	{
		mpr.success = IKEv2Route::addRouteForIKEv2();
		Logger::instance().out(L"AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE: success = %d", mpr.success);
	}
	else if (cmdId == AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES)
	{
		Logger::instance().out(L"AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES");
		RemoveWindscribeNetworkProfiles::remove();
		mpr.success = true;
	}
	else if (cmdId == AA_COMMAND_CHANGE_MTU)
	{
		CMD_CHANGE_MTU cmdChangeMtu;
		ia >> cmdChangeMtu;

		std::wstring storePersistent = L"active";
		if (cmdChangeMtu.storePersistent)
		{
			storePersistent = L"persistent";
		}

		wchar_t szBuf[1024];
		wcscpy(szBuf, L"netsh interface ipv4 set subinterface \"");
		wcscat(szBuf, cmdChangeMtu.szAdapterName.c_str());
		wcscat(szBuf, L"\" mtu=");
		wcscat(szBuf, std::to_wstring(cmdChangeMtu.mtu).c_str());
		wcscat(szBuf, L" store=");
		wcscat(szBuf, storePersistent.c_str());
		mpr = ExecuteCmd::instance().executeBlockingCmd(szBuf);

		wchar_t logBuf[1024];
		wcscpy(logBuf, L"AA_COMMAND_CHANGE_MTU: ");
		wcscat(logBuf, szBuf);
		Logger::instance().out(logBuf);

		mpr.success = true;
	}
    else if (cmdId == AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS)
    {
        mpr.success = IKEv2IPSec::setIKEv2IPSecParameters();
        Logger::instance().out(L"AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS: success = %d", mpr.success);
    }
    else if (cmdId == AA_COMMAND_START_WIREGUARD)
    {
        CMD_START_WIREGUARD cmdStartWireGuard;
        ia >> cmdStartWireGuard;
        // check input parameters
        if (Utils::isValidFileName(cmdStartWireGuard.szExecutable) &&
            Utils::noSpacesInString(cmdStartWireGuard.szDeviceName)) {
            std::wstring exePath = Utils::getExePath();
            std::wstring strCmd = L"\"" + exePath + L"\\" + cmdStartWireGuard.szExecutable + L"\"";
            strCmd += L" " + cmdStartWireGuard.szDeviceName;
            mpr = ExecuteCmd::instance().executeUnblockingCmd(strCmd.c_str(), L"", exePath.c_str());
            if (mpr.success)
                wireGuardController.init(cmdStartWireGuard.szDeviceName, mpr.blockingCmdId);
            Logger::instance().out(L"AA_COMMAND_START_WIREGUARD: cmd = %s,success = %d",
                strCmd.c_str(), mpr.success);
        }
    }
    else if (cmdId == AA_COMMAND_STOP_WIREGUARD)
    {
        Logger::instance().out(L"AA_COMMAND_STOP_WIREGUARD");
        wireGuardController.reset();
        mpr = ExecuteCmd::instance().getUnblockingCmdStatus(wireGuardController.getDaemonCmdId());
    }
    else if (cmdId == AA_COMMAND_CONFIGURE_WIREGUARD)
    {
        CMD_CONFIGURE_WIREGUARD cmdConfigureWireGuard;
        ia >> cmdConfigureWireGuard;
        if (wireGuardController.isInitialized()) {
            Logger::instance().out(L"AA_COMMAND_CONFIGURE_WIREGUARD");
            do {
                std::vector<std::string> allowed_ips_vector =
                    wireGuardController.splitAndDeduplicateAllowedIps(
                        cmdConfigureWireGuard.allowedIps);
                if (allowed_ips_vector.size() < 1) {
                    Logger::instance().out(L"invalid AllowedIps \"%s\"",
                        cmdConfigureWireGuard.allowedIps.c_str());
                    break;
                }
                if (!wireGuardController.configureAdapter(cmdConfigureWireGuard.clientIpAddress,
                    cmdConfigureWireGuard.clientDnsAddressList, allowed_ips_vector)) {
                    Logger::instance().out(L"configureAdapter() failed");
                    break;
                }
                if (!wireGuardController.configureDefaultRouteMonitor()) {
                    Logger::instance().out(L"configureDefaultRouteMonitor() failed");
                    break;
                }
                if (!wireGuardController.configureDaemon(cmdConfigureWireGuard.clientPrivateKey,
                    cmdConfigureWireGuard.peerPublicKey, cmdConfigureWireGuard.peerPresharedKey,
                    cmdConfigureWireGuard.peerEndpoint, allowed_ips_vector)) {
                    Logger::instance().out(L"configureDaemon() failed");
                    break;
                }
                mpr.success = true;
            } while (0);
        }
    }
    else if (cmdId == AA_COMMAND_GET_WIREGUARD_STATUS)
    {
        if (!wireGuardController.isInitialized()) {
            mpr.exitCode = WIREGUARD_STATE_NONE;
        } else {
            mpr = ExecuteCmd::instance().getUnblockingCmdStatus(
                wireGuardController.getDaemonCmdId());
            if (!mpr.success || mpr.blockingCmdFinished) {
                // Special error code means the daemon is dead.
                mpr.exitCode = WIREGUARD_STATE_ERROR;
                mpr.customInfoValue[0] = 666u;
            } else {
                UINT32 errorCode = 0;
                UINT64 bytesReceived = 0, bytesTransmitted = 0;
                mpr.success = true;
                mpr.exitCode =
                    wireGuardController.getStatus(&errorCode, &bytesReceived, &bytesTransmitted);
                if (mpr.exitCode == WIREGUARD_STATE_ERROR) {
                    mpr.customInfoValue[0] = errorCode;
                } else if (mpr.exitCode == WIREGUARD_STATE_ACTIVE) {
                    mpr.customInfoValue[0] = bytesReceived;
                    mpr.customInfoValue[1] = bytesTransmitted;
                }
            }
        }
    }
	
	return mpr;
}

bool writeMessagePacketResult(HANDLE hPipe, MessagePacketResult &mpr)
{
	std::stringstream stream;
	boost::archive::text_oarchive oa(stream, boost::archive::no_header);
	oa << mpr;
	const std::string str = stream.str();
	
	// first 4 bytes - size of buffer
	const unsigned long sizeOfBuf = str.size();
	if (IOUtils::writeAll(hPipe, (char *)&sizeOfBuf, sizeof(sizeOfBuf)))
	{
		if (sizeOfBuf > 0)
		{
			bool bRet = IOUtils::writeAll(hPipe, str.c_str(), sizeOfBuf);
			return bRet;
		}
	}

	return false;
}

DWORD WINAPI serviceWorkerThread(LPVOID)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
    BIND_CRASH_HANDLER_FOR_THREAD();

	FwpmWrapper	  fwpmHandleWrapper;
	if (!fwpmHandleWrapper.isInitialized())
	{
		return 0;
	}

	IcsManager            icsManager;
	FirewallFilter        firewallFilter(fwpmHandleWrapper);
	Ipv6Firewall		  ipv6Firewall(fwpmHandleWrapper);
	DnsFirewall			  dnsFirewall(fwpmHandleWrapper);
	SysIpv6Controller     sysIpv6Controller;
	HostsEdit			  hostsEdit;
	GetActiveProcesses    getActiveProcesses;
	SplitTunneling        splitTunnelling(firewallFilter, fwpmHandleWrapper);
    WireGuardController   wireGuardController(firewallFilter);

	Logger::instance().out(L"Service started");

	HANDLE hPipe = CreatePipe();
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	if (hEvent == NULL)
	{
		return 0;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//OutputDebugString(_T(
		//"My Sample Service: ServiceMain: SetServiceStatus returned error"));
	}

	OVERLAPPED overlapped;
	overlapped.hEvent = hEvent;

	HANDLE hEvents[2];

	hEvents[0] = g_ServiceStopEvent;
	hEvents[1] = hEvent;

	while (true)
	{
		::ConnectNamedPipe(hPipe, &overlapped);

		DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
		{
			break;
		}
		else if (dwWait == (WAIT_OBJECT_0 + 1))
		{
			int cmdId;
			unsigned long pid;
			unsigned long sizeOfBuf;
			if (IOUtils::readAll(hPipe, (char *)&cmdId, sizeof(cmdId)))
			{
				if (IOUtils::readAll(hPipe, (char *)&pid, sizeof(pid)))
				{
					if (IOUtils::readAll(hPipe, (char *)&sizeOfBuf, sizeof(sizeOfBuf)))
					{
						std::string strData;
						if (sizeOfBuf > 0)
						{
							std::vector<char> buffer(sizeOfBuf);
							if (IOUtils::readAll(hPipe, buffer.data(), sizeOfBuf))
							{
								strData = std::string(buffer.begin(), buffer.end());
							}
						}

						// check process executable by pid (for security), should be windscribe.exe
#ifndef _DEBUG
#ifndef SKIP_PID_CHECK
						if (Utils::verifyWindscribeProcessPath(pid))
#endif
#endif
						{
							MessagePacketResult mpr = processMessagePacket(cmdId, strData, icsManager, firewallFilter, ipv6Firewall, dnsFirewall,
																			sysIpv6Controller, hostsEdit, getActiveProcesses, splitTunnelling, wireGuardController);
							writeMessagePacketResult(hPipe, mpr);
						}

					}
				}
			}

			::FlushFileBuffers(hPipe);
			::DisconnectNamedPipe(hPipe);
		}
	}

	CloseHandle(hEvent);
	CloseHandle(hPipe);

	splitTunnelling.stop();

	CoUninitialize();
	Logger::instance().out(L"Service stopped");

	return ERROR_SUCCESS;
}
