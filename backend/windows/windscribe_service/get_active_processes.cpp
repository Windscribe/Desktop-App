#include "all_headers.h"
#include "get_active_processes.h"

GetActiveProcesses::GetActiveProcesses() : hTimer_(NULL)
{

}

GetActiveProcesses::~GetActiveProcesses()
{
	clearSavedListProcesses();
}

std::vector<std::wstring> GetActiveProcesses::getList()
{
	if (hTimer_)
	{
		DeleteTimerQueueTimer(NULL, hTimer_, INVALID_HANDLE_VALUE);
		hTimer_ = NULL;
	}

	// locked section
    {
		std::unique_lock<std::mutex> lock(mutex_);
		std::vector<DWORD> processIds;
		processIds.resize(1024);
		DWORD cbProcess;

		if (!EnumProcesses(&processIds[0], processIds.size() * sizeof(DWORD), &cbProcess))
		{
			return std::vector<std::wstring>();
		}

		DWORD cntProcessIds = cbProcess / sizeof(DWORD);

		// remove processes which finished from prev call
		for (auto it = processes_.begin(); it != processes_.end(); ++it)
		{
			it->second->bUsed = false;
		}

		for (DWORD index = 0; index < cntProcessIds; index++)
		{
			auto it = processes_.find(processIds[index]);
			if (it != processes_.end())
			{
				it->second->bUsed = true;
			}
		}

		auto it = processes_.begin();
		while (it != processes_.end())
		{
			if (!it->second->bUsed)
			{
				CloseHandle(it->second->hProcess);
				delete it->second;
				it = processes_.erase(it);
			}
			else
			{
				++it;
			}
		}

		// enumerate modules for new pids (which not in processes map)
		for (DWORD index = 0; index < cntProcessIds; index++)
		{
			auto it2 = processes_.find(processIds[index]);
			if (it2 == processes_.end())
			{
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[index]);

				if (hProcess != NULL)
				{
					HMODULE hModules[1024];
					DWORD cbNeeded;
					if (EnumProcessModulesEx(hProcess, hModules, sizeof(hModules), &cbNeeded, LIST_MODULES_ALL))
					{
                        TCHAR szProcessName[MAX_PATH];
						if (GetModuleBaseName(hProcess, hModules[0], szProcessName, MAX_PATH))
						{
							if (isSkipThisProcess(szProcessName))
							{
								CloseHandle(hProcess);
							}
							else
							{
								int len = wcslen(szProcessName);
								wcscpy(szProcessName + len - 4, L"\0");

								bool fProcessExists = false;
								int count = 0;
                                TCHAR szProcessNameWithPrefix[MAX_PATH];
								swprintf(szProcessNameWithPrefix, L"%s", szProcessName);
								do
								{
									if (count > 0)
									{
										swprintf(szProcessNameWithPrefix, L"%s#%d", szProcessName, count);
									}
									fProcessExists = isModuleExists(szProcessNameWithPrefix);
									count++;
								} while (fProcessExists);

								ProcessDescr *pd = new ProcessDescr();
								pd->bUsed = true;
								pd->hProcess = hProcess;
								pd->module = szProcessNameWithPrefix;

								processes_[processIds[index]] = pd;
							}
						}
						else  // if (GetModuleBaseName ...
						{
							CloseHandle(hProcess);
						}
					}
					else  // if (EnumProcessModulesEx ...
					{
						CloseHandle(hProcess);
					}
				}
			}
		}
	
		if (!CreateTimerQueueTimer(&hTimer_, NULL, timerCallback, this, CLEAR_SAVED_LIST_PERIOD_MS, 0, WT_EXECUTEONLYONCE))
		{
			hTimer_ = NULL;
		}

		std::vector<std::wstring> res;
		for (auto it2 = processes_.begin(); it2 != processes_.end(); ++it2)
		{
			res.push_back(it2->second->module);
		}

		return res;
	} // end of std::unique_lock<std::mutex> lock(mutex_);
}

void GetActiveProcesses::clearSavedListProcesses()
{
	if (hTimer_ != NULL)
	{
		DeleteTimerQueueTimer(NULL, hTimer_, INVALID_HANDLE_VALUE);
		hTimer_ = NULL;
	}

	{
		std::unique_lock<std::mutex> lock(mutex_);

		for (auto it = processes_.begin(); it != processes_.end(); ++it)
		{
			CloseHandle(it->second->hProcess);
			delete it->second;
		}
		processes_.clear();
	}
}

bool GetActiveProcesses::isModuleExists(TCHAR *szProcessNameWithPrefix)
{
	for (auto it = processes_.begin(); it != processes_.end(); it++)
	{
		if (it->second->module == szProcessNameWithPrefix)
		{
			return true;
		}
	}
	return false;
}
bool GetActiveProcesses::isSkipThisProcess(TCHAR *szProcessName)
{
	if (_wcsicmp(szProcessName, L"chrome.exe") == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

VOID CALLBACK GetActiveProcesses::timerCallback(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
{
	GetActiveProcesses *this_ = static_cast<GetActiveProcesses *>(lpParameter);
	std::unique_lock<std::mutex> lock(this_->mutex_);
	for (auto it = this_->processes_.begin(); it != this_->processes_.end(); ++it)
	{
		CloseHandle(it->second->hProcess);
		delete it->second;
	}
	this_->processes_.clear();
}