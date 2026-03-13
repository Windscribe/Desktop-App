// Have to include wireguard.h first or we'll get include conflicts in the Windows headers.
#include "wireguard.h"
#include "wireguardcontroller.h"

#include <Cfgmgr32.h>
#include <devguid.h>
#include <ndisguid.h>
#include <SetupAPI.h>

#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>

#include "types/global_consts.h"
#include "utils/servicecontrolmanager.h"
#include "utils/win32handle.h"
#include "utils/wsscopeguard.h"
#include "../../common/helper_commands.h"
#include "../executecmd.h"
#include "../utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif

static const DEVPROPKEY WG_DEVP_KEYNAME = DEVPKEY_WG_NAME;


WireGuardController::WireGuardController()
{
}

bool WireGuardController::installService(bool isAmneziaWG)
{
    isAmneziaWG_ = isAmneziaWG;
    isInitialized_ = false;
    try {
        exeName_ = L"WireguardService.exe";
        const std::wstring confFile = configFile();
        if (confFile.empty()) {
            return false;
        }

        std::wstring serviceCmdLine;
        {
            std::wostringstream stream;
            stream << L"\"" << Utils::getExePath() << L"\\" << exeName_ << "\" \"" << confFile << L"\"";
            if (isAmneziaWG) {
                stream << " --amneziawg";
            }
            serviceCmdLine = stream.str();
        }

        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        if (svcCtrl.isServiceInstalled(kWireGuardServiceIdentifier.c_str())) {
            spdlog::info("WireGuardController::installService - deleting existing WireGuard service");
            std::error_code ec;
            if (!svcCtrl.deleteService(kWireGuardServiceIdentifier.c_str(), ec)) {
                spdlog::error("WireGuardController::installService - failed to delete existing WireGuard service ({})", ec.value());
            }
        }

#if defined(USE_SIGNATURE_CHECK)
        ExecutableSignature sigCheck;
        std::wstring servicePath = Utils::getExePath() + L"\\" + exeName_;
        if (!sigCheck.verify(servicePath)) {
            spdlog::error("WireGuard service signature incorrect: {}", sigCheck.lastError());
            return false;
        }
#endif

        svcCtrl.installService(kWireGuardServiceIdentifier.c_str(), serviceCmdLine.c_str(),
            L"Windscribe Wireguard Tunnel", L"Manages the Windscribe WireGuard tunnel connection",
            SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, L"Nsi\0TcpIp\0", true);

        svcCtrl.setServiceSIDType(SERVICE_SID_TYPE_UNRESTRICTED);

        isInitialized_ = true;
    }
    catch (std::system_error& ex) {
        spdlog::error("WireGuardController::installService - {}", ex.what());
    }

    return isInitialized_;
}

bool WireGuardController::configure(const std::wstring &config)
{
    const std::wstring confFile = configFile();
    if (confFile.empty()) {
        return false;
    }

    std::wofstream file(confFile.c_str(), std::ios::out | std::ios::trunc);
    if (!file) {
        spdlog::error("WireGuardController::configure - could not open config file for writing");
        return false;
    }

    file << config;
    file.flush();
    file.close();

    return true;
}

bool WireGuardController::deleteService()
{
    isInitialized_ = false;

    bool bServiceDeleted = false;
    try {
        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        if (svcCtrl.isServiceInstalled(kWireGuardServiceIdentifier.c_str())) {
            spdlog::info("WireGuardController::deleteService - deleting WireGuard service");
            std::error_code ec;
            if (!svcCtrl.deleteService(kWireGuardServiceIdentifier.c_str(), ec)) {
                throw std::system_error(ec);
            }
        }

        bServiceDeleted = true;
    }
    catch (std::system_error& ex) {
        spdlog::error("WireGuardController::deleteService - {}", ex.what());
    }

    if (!bServiceDeleted && !exeName_.empty()) {
        spdlog::info("WireGuardController::deleteService - task killing the WireGuard service");
        std::wstring killCmd = Utils::getSystemDir() + L"\\taskkill.exe /f /t /im " + exeName_;
        ExecuteCmd::instance().executeBlockingCmd(killCmd);
    }

    const std::wstring confFile = configFile();
    if (!confFile.empty()) {
        std::error_code ec;
        std::filesystem::remove(confFile, ec);
    }
    return bServiceDeleted;
}

UINT WireGuardController::getStatus(ULONG64& lastHandshake, ULONG64& txBytes, ULONG64& rxBytes) const
{
    UINT result = kWgStateActive;

    lastHandshake = 0;
    txBytes = 0;
    rxBytes = 0;

    try {
        if (!isInitialized_) {
            throw std::system_error(ERROR_INVALID_STATE, std::generic_category(),
                "WireGuardController::getStatus - the WireGuard tunnel is not initialized");
        }

        if (isAmneziaWG_) {
            getAmneziaWGStatus(lastHandshake, txBytes, rxBytes);
        } else {
            getKernelDriverStatus(lastHandshake, txBytes, rxBytes);
        }
    }
    catch (std::system_error& ex) {
        result = kWgStateError;
        spdlog::error("WireGuardController::getStatus - {}", ex.what());
    }

    return result;
}

void WireGuardController::getKernelDriverStatus(ULONG64& lastHandshake, ULONG64& txBytes, ULONG64& rxBytes) const
{
    wsl::Win32Handle hDriver(getKernelInterfaceHandle());

    // Look at kernel_get_device() in wireguard-windows-0.5.3\.deps\src\ipc-windows.h for
    // sample code showing how to parse the structures returned from the wireguard-nt
    // kernel driver when we send it the WG_IOCTL_GET io control command.

    DWORD bufferSize = 4096;
    std::unique_ptr< BYTE[] > buffer(new BYTE[bufferSize]);

    // Only perform max 3 attempts, just in case we keep getting ERROR_MORE_DATA for some reason.
    BOOL apiResult = FALSE;
    for (int i = 0; !apiResult && i < 3; ++i) {
        apiResult = ::DeviceIoControl(hDriver.getHandle(), WG_IOCTL_GET, NULL, 0, buffer.get(),
                                      bufferSize, &bufferSize, NULL);
        if (!apiResult) {
            if (::GetLastError() != ERROR_MORE_DATA) {
                throw std::system_error(::GetLastError(), std::generic_category(),
                                        "WireGuardController::getStatus - DeviceIoControl failed");
            }

            buffer.reset(new BYTE[bufferSize]);
        }
    }

    if (!apiResult) {
        throw std::system_error(ERROR_UNIDENTIFIED_ERROR, std::generic_category(),
                                "WireGuardController::getStatus - DeviceIoControl failed repeatedly");
    }

    if (bufferSize < sizeof(WG_IOCTL_INTERFACE)) {
        throw std::system_error(ERROR_INVALID_DATA, std::generic_category(),
                                std::string("WireGuardController::getStatus - DeviceIoControl returned ") + std::to_string(bufferSize) +
                                    std::string(" bytes, expected ") + std::to_string(sizeof(WG_IOCTL_INTERFACE)));
    }

    WG_IOCTL_INTERFACE* wgInterface = (WG_IOCTL_INTERFACE*)buffer.get();
    if (wgInterface->PeersCount > 0) {
        if (bufferSize < sizeof(WG_IOCTL_INTERFACE) + sizeof(WG_IOCTL_PEER)) {
            throw std::system_error(ERROR_INVALID_DATA, std::generic_category(),
                                    std::string("WireGuardController::getStatus - DeviceIoControl returned ") + std::to_string(bufferSize) +
                                        std::string(" bytes, expected ") + std::to_string(sizeof(WG_IOCTL_INTERFACE) + sizeof(WG_IOCTL_PEER)));
        }

        WG_IOCTL_PEER* wgPeerInfo = (WG_IOCTL_PEER*)(buffer.get() + sizeof(WG_IOCTL_INTERFACE));
        lastHandshake = wgPeerInfo->LastHandshake;
        txBytes = wgPeerInfo->TxBytes;
        rxBytes = wgPeerInfo->RxBytes;
    }
}

HANDLE WireGuardController::getKernelInterfaceHandle() const
{
    HDEVINFO devInfo = ::SetupDiGetClassDevsExW(&GUID_DEVCLASS_NET, L"SWD\\WireGuard",
                                                NULL, DIGCF_PRESENT, NULL, NULL, NULL);
    if (devInfo == INVALID_HANDLE_VALUE) {
        throw std::system_error(::GetLastError(), std::generic_category(),
            "WireGuardController::getKernelInterfaceHandle - SetupDiGetClassDevsExW failed");
    }

    auto exitGuard = wsl::wsScopeGuard([&]
    {
        ::SetupDiDestroyDeviceInfoList(devInfo);
    });

    HANDLE hKernelInterface = INVALID_HANDLE_VALUE;

    const DWORD bufferSize = 4096;
    std::unique_ptr< BYTE[] > buffer(new BYTE[bufferSize]);

    for (DWORD i = 0; true; ++i) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        BOOL result = ::SetupDiEnumDeviceInfo(devInfo, i, &devInfoData);
        if (result == FALSE) {
            if (::GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }

            continue;
        }

        DEVPROPTYPE propType;
        DWORD requiredSize;
        result = ::SetupDiGetDevicePropertyW(devInfo, &devInfoData, &WG_DEVP_KEYNAME, &propType,
                                             buffer.get(), bufferSize, &requiredSize, 0);

        if (result == FALSE || propType != DEVPROP_TYPE_STRING) {
            continue;
        }

        // requiredSize count includes the null terminator.
        std::wstring adapterName((LPCWSTR)buffer.get(), requiredSize / sizeof(wchar_t) - 1);
        if (!Utils::iequals(adapterName, kWireGuardAdapterIdentifier)) {
            continue;
        }

        result = ::SetupDiGetDeviceInstanceIdW(devInfo, &devInfoData, (PWSTR)buffer.get(),
                                               bufferSize / sizeof(wchar_t), &requiredSize);
        if (result == FALSE) {
            throw std::system_error(::GetLastError(), std::generic_category(),
                "WireGuardController::getKernelInterfaceHandle - SetupDiGetDeviceInstanceIdW failed");
        }

        CONFIGRET confRet = ::CM_Get_Device_Interface_List_SizeW(&requiredSize, (GUID*)&GUID_DEVINTERFACE_NET,
                                                                 (DEVINSTID_W)buffer.get(), CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
        if (confRet != CR_SUCCESS) {
            throw std::system_error(0, std::generic_category(),
                std::string("WireGuardController::getKernelInterfaceHandle - CM_Get_Device_Interface_List_SizeW failed: ") + std::to_string(confRet));
        }

        std::unique_ptr< wchar_t > interfaceName(new wchar_t[requiredSize]);

        confRet = ::CM_Get_Device_Interface_ListW((GUID*)&GUID_DEVINTERFACE_NET, (DEVINSTID_W)buffer.get(),
                                                  interfaceName.get(), requiredSize, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
        if (confRet != CR_SUCCESS) {
            throw std::system_error(0, std::generic_category(),
                std::string("WireGuardController::getKernelInterfaceHandle - CM_Get_Device_Interface_ListW failed: ") + std::to_string(confRet));
        }

        size_t nameLen = wcsnlen_s(interfaceName.get(), requiredSize);

        if (nameLen == 0 || nameLen == requiredSize) {
            throw std::system_error(0, std::generic_category(),
                std::string("WireGuardController::getKernelInterfaceHandle - CM_Get_Device_Interface_ListW returned an invalid buffer: ") + std::to_string(nameLen));
        }

        hKernelInterface = ::CreateFileW(interfaceName.get(), GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         NULL, OPEN_EXISTING, 0, NULL);

        if (hKernelInterface == INVALID_HANDLE_VALUE) {
            std::wostringstream stream;
            stream << L"WireGuardController::getKernelInterfaceHandle - CreateFileW failed to open " << interfaceName.get();
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            throw std::system_error(::GetLastError(), std::generic_category(), converter.to_bytes(stream.str()));
        }

        break;
    }

    if (hKernelInterface == INVALID_HANDLE_VALUE) {
        throw std::system_error(ERROR_FILE_NOT_FOUND, std::generic_category(),
            "WireGuardController::getKernelInterfaceHandle - could not find the wireguard-nt kernel interface file descriptor");
    }

    return hKernelInterface;
}

std::wstring WireGuardController::configFile() const
{
    std::wstring confFile = Utils::getConfigPath();
    if (confFile.empty()) {
        spdlog::error("WireGuardController: could not get config file path");
        return std::wstring();
    }
    confFile += L"\\" + kWireGuardAdapterIdentifier + L".conf";
    return confFile;
}

void WireGuardController::getAmneziaWGStatus(ULONG64& lastHandshake, ULONG64& txBytes, ULONG64& rxBytes) const
{
    // Connect to the AmneziaWG UAPI named pipe
    // The adapter name is "WindscribeWireguard" as defined in ServiceMain.cpp
    std::wstring pipeName = L"\\\\.\\pipe\\ProtectedPrefix\\Administrators\\AmneziaWG\\" + kWireGuardAdapterIdentifier;

    wsl::Win32Handle hPipe(::CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL));

    if (!hPipe.isValid()) {
        throw std::system_error(::GetLastError(), std::generic_category(),
            "WireGuardController::getAmneziaWGStatus - CreateFileW failed to open named pipe");
    }

    const char* command = "get=1\n\n";
    DWORD bytesWritten = 0;
    if (!::WriteFile(hPipe.getHandle(), command, static_cast<DWORD>(strlen(command)), &bytesWritten, NULL)) {
        throw std::system_error(::GetLastError(), std::generic_category(),
            "WireGuardController::getAmneziaWGStatus - WriteFile failed");
    }

    const DWORD bufferSize = 4096;
    char buffer[bufferSize];
    DWORD bytesRead = 0;
    std::string response;

    while (true) {
        // TODO: JDRM need to ruminate on if the thread could block here and prevent us from stopping the service.
        BOOL result = ::ReadFile(hPipe.getHandle(), buffer, bufferSize - 1, &bytesRead, NULL);
        if (!result) {
            DWORD error = ::GetLastError();
            if (error == ERROR_MORE_DATA) {
                // More data available, continue reading
                buffer[bytesRead] = '\0';
                response += buffer;
                continue;
            }
            throw std::system_error(error, std::generic_category(), "WireGuardController::getAmneziaWGStatus - ReadFile failed");
        }

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response += buffer;
        }
        break;
    }

    // Parse the response for key=value pairs
    // The response format is: key=value\n pairs, ending with \n\n
    // We're looking for: rx_bytes, tx_bytes, last_handshake_time_sec
    std::istringstream stream(response);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            break; // Double newline indicates end of response
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace and carriage returns
            value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
            value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());

            if (key == "rx_bytes") {
                rxBytes = std::stoull(value);
            } else if (key == "tx_bytes") {
                txBytes = std::stoull(value);
            } else if (key == "last_handshake_time_sec") {
                // Convert Unix timestamp (seconds since 1970) to Windows FILETIME
                // (100-nanosecond intervals since 1601)
                ULONG64 unixTimestamp = std::stoull(value);
                if (unixTimestamp > 0) {
                    // Unix epoch (1970-01-01) to Windows epoch (1601-01-01) is 11644473600 seconds
                    lastHandshake = (unixTimestamp + 11644473600ULL) * 10000000ULL;
                }
            } else if (key == "errno") {
                int errorCode = std::stoi(value);
                if (errorCode != 0) {
                    throw std::system_error(errorCode, std::generic_category(),
                        "WireGuardController::getAmneziaWGStatus - UAPI returned error");
                }
            }
        }
    }
}
