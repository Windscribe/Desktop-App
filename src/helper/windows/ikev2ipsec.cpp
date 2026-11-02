#include <shlobj.h>

#include <string>
#include <spdlog/spdlog.h>

#include "ikev2ipsec.h"
#include "executecmd.h"
#include "utils.h"
#include "utils/win32handle.h"

namespace
{
static const TCHAR kConnectionName[] = TEXT(WS_WIN_IKEV2_CONNECTION_NAME);

// RAII helper for security impersonation as the user owning the helper IPC pipe client.
// This is essential because phone books are specific to the user, and the RAS entry is created
// by the client process, which is not necessarily running as the active console session's user
// (e.g. RDP, fast user switching, runas).
class PipeClientImpersonationHelper
{
public:
    explicit PipeClientImpersonationHelper(HANDLE clientPipe)
    {
        if (!Utils::getPipeClientPrimaryToken(clientPipe, token_.data())) {
            return;
        }
        if (!ImpersonateLoggedOnUser(token_.getHandle())) {
            spdlog::error("PipeClientImpersonationHelper: ImpersonateLoggedOnUser failed {}", ::GetLastError());
            token_.closeHandle();
        }
    }
    ~PipeClientImpersonationHelper()
    {
        if (isImpersonated()) {
            if (!RevertToSelf()) {
                spdlog::error("PipeClientImpersonationHelper: RevertToSelf failed {}", ::GetLastError());
            }
        }
    }
    HANDLE token() const { return token_.getHandle(); }
    bool isImpersonated() const { return token_.isValid(); }
private:
    wsl::Win32Handle token_;
};
}

bool IKEv2IPSec::setIKEv2IPSecParameters(HANDLE clientPipe)
{
    disableMODP2048Support();

    // First, try to add IPSec parameters to the phonebook manually.
    if (setIKEv2IPSecParametersInPhoneBook(clientPipe))
        return true;

    spdlog::warn(L"setIKEv2IPSecParameters could not edit the RAS phonebook, falling back to PowerShell.");
    return setIKEv2IPSecParametersPowerShell(clientPipe);
}

bool IKEv2IPSec::setIKEv2IPSecParametersInPhoneBook(HANDLE clientPipe)
{
    PipeClientImpersonationHelper impersonation_helper(clientPipe);
    if (!impersonation_helper.isImpersonated())
        return false;

    TCHAR pbk_path[MAX_PATH];
    for (const auto clsid : { CSIDL_APPDATA, CSIDL_LOCAL_APPDATA, CSIDL_COMMON_APPDATA }) {
        HRESULT folderHr = SHGetFolderPath(0, clsid, impersonation_helper.token(), SHGFP_TYPE_CURRENT, pbk_path);
        if (FAILED(folderHr)) {
            spdlog::debug("setIKEv2IPSecParametersInPhoneBook: SHGetFolderPath({}) failed: {:#x}", clsid, static_cast<unsigned long>(folderHr));
            continue;
        }
        std::wstring phonebookEntry(pbk_path);
        phonebookEntry += L"\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk";
        // Check if the phonebook exists and is valid.
        auto pbk_handle = CreateFile(phonebookEntry.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pbk_handle == INVALID_HANDLE_VALUE) {
            const DWORD openError = ::GetLastError();
            if (openError != ERROR_FILE_NOT_FOUND && openError != ERROR_PATH_NOT_FOUND) {
                spdlog::debug(L"setIKEv2IPSecParametersInPhoneBook: cannot open phonebook {} ({})", phonebookEntry, openError);
            }
            continue;
        }
        CloseHandle(pbk_handle);
        // Only modify the phonebook that actually holds our RAS entry. Otherwise we would add an
        // orphan section to an unrelated phonebook and never update the real entry.
        TCHAR key_names[64] = {};
        if (GetPrivateProfileString(kConnectionName, nullptr, L"", key_names, _countof(key_names),
                                    phonebookEntry.c_str()) == 0)
            continue;
        // Write custom IPSec parameters to the phonebook.
        if (!WritePrivateProfileString(
            kConnectionName, L"NumCustomPolicy", L"1", phonebookEntry.c_str()) ||
            // Note that this value is approximately, but not exactly, a ROUTER_CUSTOM_IKEv2_POLICY0 structure,
            // with each DWORD written out one byte at a time.  This value was actually derived from setting
            // the policy with the PowerShell command and then inspecting the rasphone.pbk file.
            !WritePrivateProfileString(
                kConnectionName, L"CustomIPSecPolicies",
                L"030000000600000005000000080000000500000005000000", phonebookEntry.c_str()) ||
            // This string disables NetBIOS over TCP/IP.
            !WritePrivateProfileString( kConnectionName, L"IpNBTFlags", L"0", phonebookEntry.c_str())) {
            // This is a valid phonebook, but we cannot write it. Don't try other locations, they
            // won't make any sense; better to try other IPSec setup functions, like PowerShell.
            // Short-circuit evaluation guarantees the failing WritePrivateProfileString was the last call.
            const DWORD writeError = ::GetLastError();
            spdlog::warn(L"Phonebook is not accessible: {} ({})", phonebookEntry.c_str(), writeError);
            break;
        }
        return true;
    }
    return false;
}

bool IKEv2IPSec::setIKEv2IPSecParametersPowerShell(HANDLE clientPipe)
{
    TCHAR command_buffer[1024] = {};
    wsprintf(command_buffer, L"PowerShell -Command \"Set-VpnConnectionIPsecConfiguration"
        " -ConnectionName '%ls'"
        " -AuthenticationTransformConstants GCMAES256 -CipherTransformConstants GCMAES256"
        " -EncryptionMethod GCMAES256 -IntegrityCheckMethod SHA384"
        " -DHGroup ECP384 -PfsGroup ECP384 -Force\"", kConnectionName);

    PipeClientImpersonationHelper impersonation_helper(clientPipe);
    if (!impersonation_helper.isImpersonated())
        return false;

    auto mpr = ExecuteCmd::instance().executeBlockingCmd(
        command_buffer, impersonation_helper.token());
    if (!mpr.success || mpr.exitCode != 0) {
        spdlog::error(L"Command failed (exit code {}): {}", mpr.exitCode, command_buffer);
        if (!mpr.output.empty())
            spdlog::info(L"Output: {}", mpr.output);
        return false;
    }
    return true;
}

void IKEv2IPSec::disableMODP2048Support()
{
    // RasDial in the client will fail with error code 13868 if this registry setting is enabled.
    // See https://wiki.strongswan.org/projects/strongswan/wiki/WindowsClients for more info on this setting.

    HKEY hKey = NULL;
    LSTATUS result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\Rasman\\Parameters",
                                    0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        DWORD dwValueType = REG_DWORD;
        DWORD dwValue;
        DWORD dwValueSize = sizeof(dwValue);
        result = ::RegQueryValueEx(hKey, L"NegotiateDH2048_AES256", NULL, &dwValueType, (LPBYTE)&dwValue, &dwValueSize);

        if (result == ERROR_SUCCESS) {
            if (dwValue != 0) {
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
