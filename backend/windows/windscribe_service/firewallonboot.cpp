#include "all_headers.h"

#include "firewallonboot.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "utils.h"
#include "registry.h"

FirewallOnBootManager::FirewallOnBootManager()
{
}

FirewallOnBootManager::~FirewallOnBootManager()
{
}

bool FirewallOnBootManager::setEnabled(bool enabled)
{
    if (enabled) {
        return enable();
    }
    return disable();
}

bool FirewallOnBootManager::isEnabled()
{
    DWORD value = 0;
    DWORD size = sizeof(DWORD);

    bool result = Registry::regGetProperty(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Windscribe\\Windscribe2", L"FirewallOnBoot", (LPBYTE)&value, &size);

    return result && value == 1;
}

bool FirewallOnBootManager::enable()
{
    bool result = Registry::regWriteDwordProperty(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Windscribe\\Windscribe2", L"FirewallOnBoot", 1);

    if (result) {
        spdlog::info("Firewall on boot enabled");
    } else {
        spdlog::error("Failed to enable firewall on boot");
    }

    return result;
}

bool FirewallOnBootManager::disable()
{
    bool result = Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Windscribe\\Windscribe2", L"FirewallOnBoot");

    if (result) {
        spdlog::info("Firewall on boot disabled");
    } else {
        spdlog::error("Failed to disable firewall on boot");
    }

    return result;
}
