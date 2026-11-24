#include "delete_user_data.h"

#include <Windows.h>
#include <KnownFolders.h>
#include <netfw.h>
#include <ShlObj.h>
#include <wrl/client.h>

#include <string>
#include <vector>
#include <spdlog/spdlog.h>

#include "remove_directory.h"
#include "../utils/applicationinfo.h"
#include "../utils/interactive_user.h"
#include "../utils/path.h"

#include "ws_branding.h"
#include "wsscopeguard.h"

using Microsoft::WRL::ComPtr;

namespace DeleteUserData
{

namespace {

const std::wstring kOrgKey = L"Software\\" WS_SETTINGS_ORG_W;
const std::wstring kAppKey = kOrgKey + L"\\" WS_SETTINGS_APP_W;
const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kStartupApprovedRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";
// The app's multiple-account (abuse) detection record: the username, user ID and date of a free
// account whose session expired.  Keep in sync with g_szKeyPath in secretvalue_win.cpp.
const wchar_t kAccountDetectionKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\NetControlPort";

bool isGone(LSTATUS status)
{
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

// Deletes the key, its subkeys and its values.  hiveName is only for logging.
bool deleteKeyTree(HKEY root, const std::wstring &subKey, const wchar_t *hiveName)
{
    const LSTATUS status = ::RegDeleteTreeW(root, subKey.c_str());
    if (!isGone(status)) {
        spdlog::error(L"Failed to delete registry key {}\\{} ({})", hiveName, subKey, status);
        return false;
    }
    return true;
}

// Deletes the key only if nothing is left in it: other products from the same vendor may share
// the organization key.
void deleteKeyIfEmpty(HKEY root, const std::wstring &subKey, const wchar_t *hiveName)
{
    HKEY key = nullptr;
    if (::RegOpenKeyExW(root, subKey.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return;
    }

    DWORD subKeyCount = 0;
    DWORD valueCount = 0;
    const LSTATUS status = ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, &subKeyCount, nullptr, nullptr,
                                              &valueCount, nullptr, nullptr, nullptr, nullptr);
    ::RegCloseKey(key);

    if (status == ERROR_SUCCESS && subKeyCount == 0 && valueCount == 0) {
        const LSTATUS deleteStatus = ::RegDeleteKeyW(root, subKey.c_str());
        if (!isGone(deleteStatus)) {
            spdlog::warn(L"Failed to delete empty registry key {}\\{} ({})", hiveName, subKey, deleteStatus);
        }
    }
}

bool deleteValue(HKEY root, const wchar_t *subKey, const wchar_t *valueName, const wchar_t *hiveName)
{
    const LSTATUS status = ::RegDeleteKeyValueW(root, subKey, valueName);
    if (!isGone(status)) {
        spdlog::error(L"Failed to delete registry value {}\\{}\\{} ({})", hiveName, subKey, valueName, status);
        return false;
    }
    return true;
}

// The settings, account-detection record and launch-on-startup entries in one user's hive.  root
// is that user's HKEY_CURRENT_USER.
bool deleteUserRegistryData(HKEY root, const wchar_t *hiveName)
{
    bool ok = true;
    ok = deleteKeyTree(root, kAppKey, hiveName) && ok;
    ok = deleteKeyTree(root, ApplicationInfo::installerRegistryKey(false), hiveName) && ok;
    deleteKeyIfEmpty(root, kOrgKey, hiveName);
    ok = deleteKeyTree(root, kAccountDetectionKey, hiveName) && ok;

    // Written by the app when "Launch on startup" is enabled (see launchonstartup_win.cpp).  Explorer
    // keys StartupApproved entries by the Run value's name, so remove that one as well as the one
    // the app writes.
    ok = deleteValue(root, kRunKey, WS_PRODUCT_NAME_W, hiveName) && ok;
    ok = deleteValue(root, kStartupApprovedRunKey, WS_APP_IDENTIFIER_W, hiveName) && ok;
    if (std::wstring(WS_APP_IDENTIFIER_W) != WS_PRODUCT_NAME_W) {
        ok = deleteValue(root, kStartupApprovedRunKey, WS_PRODUCT_NAME_W, hiveName) && ok;
    }

    return ok;
}

bool exists(const std::wstring &path)
{
    if (::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    const DWORD err = ::GetLastError();
    return err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND;
}

// Must run while impersonating the user who owns token, with COM initialized (see run()).
bool deleteUserFiles(HANDLE token)
{
    PWSTR folder = nullptr;
    const HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DONT_VERIFY, token, &folder);
    if (FAILED(hr)) {
        ::CoTaskMemFree(folder);
        spdlog::error(L"Failed to locate the user's local app data folder ({:#x})", static_cast<unsigned long>(hr));
        return false;
    }
    const std::wstring localAppData(folder);
    ::CoTaskMemFree(folder);

    bool ok = true;

    // Qt's AppLocalDataLocation: logs, custom configs, ctrld data, crash dumps, update downloads.
    const std::wstring orgDir = Path::append(localAppData, WS_SETTINGS_ORG_W);
    const std::wstring appDir = Path::append(orgDir, WS_SETTINGS_APP_W);
    if (exists(appDir)) {
        spdlog::info(L"Deleting {}", appDir);
        if (!RemoveDir::DelTree(appDir, true, true, true, false)) {
            spdlog::error(L"Failed to fully delete {} ({})", appDir, ::GetLastError());
            ok = false;
        }
    }
    // Succeeds only if empty: other products from the same vendor may keep data here.
    ::RemoveDirectoryW(orgDir.c_str());

    // Earlier versions kept the advanced parameters file directly in %LOCALAPPDATA%.
    const std::wstring legacyExtraConfig = Path::append(localAppData, WS_EXTRA_CONFIG_NAME_W);
    if (!::DeleteFileW(legacyExtraConfig.c_str())) {
        const DWORD err = ::GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            spdlog::error(L"Failed to delete {} ({})", legacyExtraConfig, err);
            ok = false;
        }
    }

    return ok;
}

bool deleteInteractiveUserData(const wsl::InteractiveUser &user)
{
    wsl::ScopedImpersonation impersonation(user.token());
    if (!impersonation.isActive()) {
        spdlog::error(L"Failed to impersonate the interactive user ({}); their settings and data were not deleted",
                      impersonation.error());
        return false;
    }

    // HKEY_CURRENT_USER would still be the uninstaller's own hive while impersonating.
    HKEY userHive = nullptr;
    const LSTATUS status = ::RegOpenCurrentUser(KEY_READ | KEY_WRITE | DELETE, &userHive);

    bool ok = true;
    if (status == ERROR_SUCCESS) {
        ok = deleteUserRegistryData(userHive, L"HKCU") && ok;
        ::RegCloseKey(userHive);
    } else {
        spdlog::error(L"Failed to open the interactive user's registry hive ({})", status);
        ok = false;
    }

    ok = deleteUserFiles(user.token()) && ok;
    return ok;
}

class ScopedBstr
{
public:
    explicit ScopedBstr(const std::wstring &str) : bstr_(::SysAllocStringLen(str.data(), static_cast<UINT>(str.size()))) {}
    ScopedBstr() = default;
    ~ScopedBstr() { ::SysFreeString(bstr_); }
    ScopedBstr(const ScopedBstr &) = delete;
    ScopedBstr &operator=(const ScopedBstr &) = delete;

    BSTR get() const { return bstr_; }
    BSTR *receive() { ::SysFreeString(bstr_); bstr_ = nullptr; return &bstr_; }
    std::wstring str() const { return bstr_ ? std::wstring(bstr_, ::SysStringLen(bstr_)) : std::wstring(); }

private:
    BSTR bstr_ = nullptr;
};

// Rules may store their program with environment variables (e.g. %ProgramFiles%) or in a
// different case; the executable no longer exists, so compare normalized strings.
std::wstring normalizeProgramPath(const std::wstring &path)
{
    std::wstring result = path;

    const DWORD expandedSize = ::ExpandEnvironmentStringsW(path.c_str(), nullptr, 0);
    if (expandedSize > 0) {
        std::wstring expanded(expandedSize, L'\0');
        const DWORD written = ::ExpandEnvironmentStringsW(path.c_str(), expanded.data(), expandedSize);
        if (written > 0 && written <= expandedSize) {
            expanded.resize(written - 1);
            result = expanded;
        }
    }

    const DWORD fullSize = ::GetFullPathNameW(result.c_str(), 0, nullptr, nullptr);
    if (fullSize > 0) {
        std::wstring full(fullSize, L'\0');
        const DWORD written = ::GetFullPathNameW(result.c_str(), fullSize, full.data(), nullptr);
        if (written > 0 && written < fullSize) {
            full.resize(written);
            result = full;
        }
    }

    return result;
}

bool isSameProgram(const std::wstring &a, const std::wstring &b)
{
    return ::CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
}

struct FirewallRule
{
    ComPtr<INetFwRule> rule;
    std::wstring name;
    NET_FW_RULE_DIRECTION direction = NET_FW_RULE_DIR_IN;
    NET_FW_ACTION action = NET_FW_ACTION_BLOCK;
};

std::wstring describe(const FirewallRule &r)
{
    return L"'" + r.name + L"' (" + (r.direction == NET_FW_RULE_DIR_IN ? L"inbound" : L"outbound") + L", " +
           (r.action == NET_FW_ACTION_ALLOW ? L"allow" : L"block") + L")";
}

// Windows Defender Firewall rules for the app.  Windows creates these itself, from the "Windows
// Security Alert" prompt the first time the app accepts incoming connections.  They outlive the
// app, and would apply again to a copy reinstalled at the same path (e.g. blocking Proxy Gateway
// clients).  Only rules whose program is our executable are deleted.  COM must be initialized on
// this thread (see run()).
bool deleteWindowsFirewallRules(const std::wstring &installationPath)
{
    const std::wstring program = normalizeProgramPath(Path::append(installationPath, ApplicationInfo::appExeName()));

    ComPtr<INetFwPolicy2> policy;
    HRESULT hr = ::CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&policy));
    ComPtr<INetFwRules> rules;
    if (SUCCEEDED(hr)) {
        hr = policy->get_Rules(&rules);
    }
    ComPtr<IUnknown> enumUnknown;
    if (SUCCEEDED(hr)) {
        hr = rules->get__NewEnum(&enumUnknown);
    }
    ComPtr<IEnumVARIANT> enumerator;
    if (SUCCEEDED(hr)) {
        hr = enumUnknown.As(&enumerator);
    }
    if (FAILED(hr)) {
        spdlog::error(L"Failed to access the Windows Firewall rules ({:#x})", static_cast<unsigned long>(hr));
        return false;
    }

    // Collect first: removing rules while enumerating would invalidate the enumerator.
    std::vector<FirewallRule> matches;
    for (;;) {
        VARIANT item;
        ::VariantInit(&item);
        ULONG fetched = 0;
        hr = enumerator->Next(1, &item, &fetched);
        if (hr != S_OK || fetched == 0) {
            ::VariantClear(&item);
            break;
        }

        ComPtr<INetFwRule> rule;
        if (item.vt == VT_DISPATCH && item.pdispVal != nullptr &&
            SUCCEEDED(item.pdispVal->QueryInterface(IID_PPV_ARGS(&rule)))) {
            ScopedBstr application;
            if (SUCCEEDED(rule->get_ApplicationName(application.receive())) && application.get() != nullptr &&
                isSameProgram(normalizeProgramPath(application.str()), program)) {
                FirewallRule match;
                match.rule = rule;
                ScopedBstr name;
                if (SUCCEEDED(rule->get_Name(name.receive()))) {
                    match.name = name.str();
                }
                rule->get_Direction(&match.direction);
                rule->get_Action(&match.action);
                matches.push_back(std::move(match));
            }
        }
        ::VariantClear(&item);
    }

    bool ok = true;
    if (FAILED(hr)) {
        spdlog::error(L"Failed to enumerate the Windows Firewall rules ({:#x}); some rules for {} may remain",
                      static_cast<unsigned long>(hr), program);
        ok = false;
    }

    if (matches.empty()) {
        spdlog::info(L"No Windows Firewall rules for {}", program);
        return ok;
    }

    for (const FirewallRule &match : matches) {
        // INetFwRules::Remove takes a name, and rule names are not unique (another rule may also
        // be called "Windscribe").  Give this rule a unique name first, so that only it is removed.
        GUID guid;
        wchar_t guidString[40] = {};
        hr = ::CoCreateGuid(&guid);
        if (SUCCEEDED(hr) && ::StringFromGUID2(guid, guidString, ARRAYSIZE(guidString)) == 0) {
            hr = E_UNEXPECTED;
        }
        if (FAILED(hr)) {
            spdlog::error(L"Failed to create a unique name to delete the Windows Firewall rule {} ({:#x})",
                          describe(match), static_cast<unsigned long>(hr));
            ok = false;
            continue;
        }

        // Keep the product name, so that a rule left behind by a failed Remove stays recognizable.
        const std::wstring uniqueNameStr = std::wstring(WS_PRODUCT_NAME_W L" uninstall ") + guidString;
        const ScopedBstr uniqueName(uniqueNameStr);
        hr = match.rule->put_Name(uniqueName.get());
        if (FAILED(hr)) {
            spdlog::error(L"Failed to rename the Windows Firewall rule {} for deletion ({:#x})", describe(match),
                          static_cast<unsigned long>(hr));
            ok = false;
            continue;
        }

        hr = rules->Remove(uniqueName.get());
        if (FAILED(hr)) {
            spdlog::error(L"Failed to delete the Windows Firewall rule {}, now named '{}' ({:#x})", describe(match),
                          uniqueNameStr, static_cast<unsigned long>(hr));
            ok = false;
            continue;
        }

        spdlog::info(L"Deleted the Windows Firewall rule {} for {}", describe(match), program);
    }

    return ok;
}

bool deleteMachineData(const std::wstring &installationPath)
{
    // The helper keeps its settings (e.g. FirewallOnBoot) here; see firewallonboot.cpp.
    bool ok = deleteKeyTree(HKEY_LOCAL_MACHINE, kAppKey, L"HKLM");
    deleteKeyIfEmpty(HKEY_LOCAL_MACHINE, kOrgKey, L"HKLM");
    ok = deleteWindowsFirewallRules(installationPath) && ok;
    return ok;
}

}

bool run(const std::wstring &installationPath)
{
    spdlog::info(L"Deleting the user's settings and data");
    bool ok = true;

    // SHGetKnownFolderPath (the user's %LOCALAPPDATA%) and the Windows Firewall API both require
    // COM on the calling thread, and PerformUninstall() has already uninitialized it.  Every COM
    // pointer is local to a callee, so all are released before CoUninitialize.  If COM cannot be
    // initialized, carry on: the registry cleanup does not need it, and the steps that do log
    // their own failures.
    const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        spdlog::error(L"Failed to initialize COM ({:#x})", static_cast<unsigned long>(comResult));
    }
    auto comGuard = wsl::wsScopeGuard([comResult] {
        if (SUCCEEDED(comResult)) {
            ::CoUninitialize();
        }
    });

    wsl::InteractiveUser user;
    const DWORD err = user.open();
    if (err == ERROR_SUCCESS) {
        spdlog::info(L"Deleting the settings and data of the interactive user {}", user.sidString());
        ok = deleteInteractiveUserData(user) && ok;
    } else {
        // Never fall back to deleting from the profile as administrator.
        spdlog::warn(L"Could not identify the interactive user ({}); their settings and data were not deleted", err);
        ok = false;
    }

    // The elevated installer writes its keys to the hive of the account it runs as.  When that is
    // not the interactive user -- a standard user who entered an administrator's credentials, or a
    // management tool running as SYSTEM -- clean that hive too.  It belongs to the administrator or
    // SYSTEM, so the uninstaller's full rights are appropriate there.  This also runs when the
    // interactive user could not be identified, which is how the uninstaller already treated its
    // own HKCU before this option existed.
    if (!user.isProcessUser()) {
        spdlog::info(L"Deleting the settings of the account the uninstaller runs as");
        ok = deleteUserRegistryData(HKEY_CURRENT_USER, L"HKCU (uninstaller account)") && ok;
    }

    ok = deleteMachineData(installationPath) && ok;

    if (ok) {
        spdlog::info(L"The user's settings and data were deleted");
    } else {
        spdlog::warn(L"Deleting the user's settings and data finished with errors");
    }
    return ok;
}

}
