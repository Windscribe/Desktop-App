#include "all_headers.h"
#include "fwpm_wrapper.h"
#include "logger.h"

FwpmWrapper::FwpmWrapper()
{
	DWORD dwFwAPiRetCode = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engineHandle_);
	if (dwFwAPiRetCode != ERROR_SUCCESS)
	{
        Logger::instance().out(L"EngineHandleWrapper::EngineHandleWrapper(), FwpmEngineOpen0 failed, %lu", dwFwAPiRetCode);
		engineHandle_ = NULL;
	}
}

FwpmWrapper::~FwpmWrapper()
{
	if (engineHandle_)
	{
		FwpmEngineClose0(engineHandle_);
	}
}

bool FwpmWrapper::isInitialized()
{
	return engineHandle_ != NULL;
}

HANDLE FwpmWrapper::getHandleAndLock()
{
	mutex_.lock();
	return engineHandle_;
}

void FwpmWrapper::unlock()
{
	mutex_.unlock();
}

bool FwpmWrapper::beginTransaction()
{
	DWORD dwFwAPiRetCode = FwpmTransactionBegin0(engineHandle_, NULL);
	if (dwFwAPiRetCode != ERROR_SUCCESS)
	{
        Logger::instance().out(L"FwpmWrapper::beginTransaction(), FwpmTransactionBegin0 failed, %lu", dwFwAPiRetCode);
		return false;
	}
	return true;
}

bool FwpmWrapper::endTransaction()
{
	DWORD dwFwAPiRetCode = FwpmTransactionCommit0(engineHandle_);
	if (dwFwAPiRetCode != ERROR_SUCCESS)
	{
        Logger::instance().out(L"FwpmWrapper::endTransaction(), FwpmTransactionCommit0 failed, %lu", dwFwAPiRetCode);
		return false;
	}
	return true;
}
