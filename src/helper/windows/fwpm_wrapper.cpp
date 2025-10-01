#include "fwpm_wrapper.h"

#include <fwpmu.h>
#include <spdlog/spdlog.h>

FwpmWrapper::FwpmWrapper()
{
}

FwpmWrapper::~FwpmWrapper()
{
    release();
}

bool FwpmWrapper::initialize()
{
    DWORD dwFwAPiRetCode = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engineHandle_);
    if (dwFwAPiRetCode != ERROR_SUCCESS) {
        spdlog::error("EngineHandleWrapper::EngineHandleWrapper(), FwpmEngineOpen0 failed, {}", dwFwAPiRetCode);
        engineHandle_ = nullptr;
    }

    return engineHandle_ != nullptr;
}

void FwpmWrapper::release()
{
    if (engineHandle_) {
        FwpmEngineClose0(engineHandle_);
        engineHandle_ = nullptr;
    }
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
    if (dwFwAPiRetCode != ERROR_SUCCESS) {
        spdlog::error("FwpmWrapper::beginTransaction(), FwpmTransactionBegin0 failed, {}", dwFwAPiRetCode);
        return false;
    }
    return true;
}

bool FwpmWrapper::endTransaction()
{
    DWORD dwFwAPiRetCode = FwpmTransactionCommit0(engineHandle_);
    if (dwFwAPiRetCode != ERROR_SUCCESS) {
        spdlog::error("FwpmWrapper::endTransaction(), FwpmTransactionCommit0 failed, {}", dwFwAPiRetCode);
        return false;
    }
    return true;
}
