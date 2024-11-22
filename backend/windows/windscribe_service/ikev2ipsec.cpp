#include "all_headers.h"
#include <spdlog/spdlog.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include "ikev2ipsec.h"
#include "executecmd.h"

#pragma comment(lib, "Wtsapi32.lib")

namespace
{
static const TCHAR kWindscribeConnectionName[] = TEXT("Windscribe IKEv2");

// RAII helper for security impersonation as an active logged on user.
// This is essential because phone books are specific to the user.
class CurrentUserImpersonationHelper
{
public:
    CurrentUserImpersonationHelper() : token_(INVALID_HANDLE_VALUE), is_impersonated_(false)
    {
        HANDLE base_token;
        if (!WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &base_token))
            return;
        if (DuplicateTokenEx(base_token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
            TokenPrimary, &token_)) {
            if (ImpersonateLoggedOnUser(token_))
                is_impersonated_ = true;
        }
        CloseHandle(base_token);
    }
    ~CurrentUserImpersonationHelper()
    {
        if (is_impersonated_)
            RevertToSelf();
        if (token_ != INVALID_HANDLE_VALUE)
            CloseHandle(token_);
    }
    HANDLE token() const { return token_; }
    bool isImpersonated() const { return is_impersonated_; }
private:
    HANDLE token_;
    bool is_impersonated_;
};
}

bool IKEv2IPSec::setIKEv2IPSecParameters()
{
    disableMODP2048Support();

    // First, try to add IPSec parameters to the phonebook manually.
    if (setIKEv2IPSecParametersInPhoneBook())
        return true;
    // If it failed, try to add them via a PowerShell command.
    return setIKEv2IPSecParametersPowerShell();
}

bool IKEv2IPSec::setIKEv2IPSecParametersInPhoneBook()
{
    CurrentUserImpersonationHelper impersonation_helper;
    if (!impersonation_helper.isImpersonated())
        return false;

    TCHAR pbk_path[MAX_PATH];
    for (const auto clsid : { CSIDL_APPDATA, CSIDL_LOCAL_APPDATA, CSIDL_COMMON_APPDATA }) {
        if (SHGetFolderPath(0, clsid, impersonation_helper.token(), SHGFP_TYPE_CURRENT, pbk_path))
            continue;
        wcscat_s(pbk_path, L"\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk");
        // Check if the phonebook exists and is valid.
        auto pbk_handle = CreateFile(pbk_path, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pbk_handle == INVALID_HANDLE_VALUE)
            continue;
        CloseHandle(pbk_handle);
        // Write custom IPSec parameters to the phonebook.
        if (!WritePrivateProfileString(
            kWindscribeConnectionName, L"NumCustomPolicy", L"1", pbk_path) ||
            // Note that this value is approximately, but not exactly, a ROUTER_CUSTOM_IKEv2_POLICY0 structure,
            // with each DWORD written out one byte at a time.  This value was actually derived from setting
            // the policy with the PowerShell command and then inspecting the rasphone.pbk file.
            !WritePrivateProfileString(
                kWindscribeConnectionName, L"CustomIPSecPolicies",
                L"030000000600000005000000080000000500000005000000", pbk_path) ||
            // This string disables NetBIOS over TCP/IP.
            !WritePrivateProfileString( kWindscribeConnectionName, L"IpNBTFlags", L"0", pbk_path)) {
            // This is a valid phonebook, but we cannot write it. Don't try other locations, they
            // won't make any sense; better to try other IPSec setup functions, like PowerShell.
            spdlog::warn(L"Phonebook is not accessible: {}", pbk_path);
            break;
        }
        return true;
    }
    return false;
}

bool IKEv2IPSec::setIKEv2IPSecParametersPowerShell()
{
    TCHAR command_buffer[1024] = {};
    wsprintf(command_buffer, L"PowerShell -Command \"Set-VpnConnectionIPsecConfiguration"
        " -ConnectionName '%ls'"
        " -AuthenticationTransformConstants GCMAES256 -CipherTransformConstants GCMAES256"
        " -EncryptionMethod GCMAES256 -IntegrityCheckMethod SHA384"
        " -DHGroup ECP384 -PfsGroup ECP384 -Force\"", kWindscribeConnectionName);

    CurrentUserImpersonationHelper impersonation_helper;
    if (!impersonation_helper.isImpersonated())
        return false;

    auto mpr = ExecuteCmd::instance().executeBlockingCmd(
        command_buffer, impersonation_helper.token());
    if (!mpr.success) {
        spdlog::error(L"Command failed: {}", command_buffer);
        if (!mpr.additionalString.empty())
            spdlog::info("Output: {}", mpr.additionalString);
    }
    return mpr.success;
}

void IKEv2IPSec::disableMODP2048Support()
{
    // RasDial in the client will fail with error code 13868 if this registry setting is enabled.
    // See https://wiki.strongswan.org/projects/strongswan/wiki/WindowsClients for more info on this setting.

    HKEY hKey = NULL;
    LSTATUS result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\Rasman\\Parameters",
                                    0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        DWORD dwValueType = REG_DWORD;
        DWORD dwValue;
        DWORD dwValueSize = sizeof(dwValue);
        result = ::RegQueryValueEx(hKey, L"NegotiateDH2048_AES256", NULL, &dwValueType, (LPBYTE)&dwValue, &dwValueSize);

        if (result == ERROR_SUCCESS)
        {
            if (dwValue != 0)
            {
                dwValue = 0;
                result = ::RegSetValueEx(hKey, L"NegotiateDH2048_AES256", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
                if (result != ERROR_SUCCESS) {
                    spdlog::error("IKEv2IPSec::disableMODP2048Support: failed to set the 'NegotiateDH2048_AES256' value ({})", result);
                }
            }
        }
        else if (result != ERROR_FILE_NOT_FOUND) {
            spdlog::error("IKEv2IPSec::disableMODP2048Support: failed to query the 'NegotiateDH2048_AES256' value ({})", result);
        }

        ::RegCloseKey(hKey);
    }
    else {
        spdlog::error("IKEv2IPSec::disableMODP2048Support: failed to open the 'Rasman Parameters' key ({})", result);
    }
}
