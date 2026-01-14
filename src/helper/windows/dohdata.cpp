#include "dohdata.h"

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

static const std::wstring kDohKeyPath = L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters";
static const std::wstring kEnableDohValue = L"EnableAutoDoh";

void DohData::disableDohSettings()
{
    // In order to disable doh on Windows it is necessary to set to 0 value of the EnableAutoDoh property
    // in the registry at the address SYSTEM\CurrentControlSet\Services\Dnscache\Parameters.

    if (!lastTimeDohWasEnabled_) {
        return;
    }

    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, kDohKeyPath);
    if (!result) {
        spdlog::error(L"disableDohSettings failed opening key {}: error {}", kDohKeyPath, result.Code());
        return;
    }

    const auto value = registry.TryGetDwordValue(kEnableDohValue);
    if (!value) {
        if (value.GetError().Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"disableDohSettings failed checking value {}: error {}", kEnableDohValue, value.GetError().Code());
            return;
        }
        result = registry.TrySetDwordValue(kEnableDohValue, 0);
        if (!result) {
            spdlog::error(L"disableDohSettings failed to create value {}: error {}", kEnableDohValue, result.Code());
            return;
        }
        dohRegistryWasCreated_ = true;
        enableAutoDoh_ = 0;
    } else {
        dohRegistryWasCreated_ = false;
        enableAutoDoh_ = value.GetValue();
        result = registry.TrySetDwordValue(kEnableDohValue, 0);
        if (!result) {
            spdlog::error(L"disableDohSettings failed to write zero to value {}: error {}", kEnableDohValue, result.Code());
            return;
        }
    }

    lastTimeDohWasEnabled_ = false;
}

void DohData::enableDohSettings()
{
    if (lastTimeDohWasEnabled_) {
        return;
    }

    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, kDohKeyPath);
    if (!result) {
        spdlog::error(L"enableDohSettings failed opening key {}: error {}", kDohKeyPath, result.Code());
        return;
    }

    if (dohRegistryWasCreated_) {
        result = registry.TryDeleteValue(kEnableDohValue);
        if (!result) {
            spdlog::error(L"enableDohSettings failed to delete value {}: error {}", kEnableDohValue, result.Code());
            return;
        }
        dohRegistryWasCreated_ = false;
        lastTimeDohWasEnabled_ = true;
    } else {
        result = registry.TrySetDwordValue(kEnableDohValue, enableAutoDoh_);
        if (!result) {
            spdlog::error(L"enableDohSettings failed to set value {} to {}: error {}", kEnableDohValue, enableAutoDoh_, result.Code());
            return;
        }
        lastTimeDohWasEnabled_ = true;
    }
}
