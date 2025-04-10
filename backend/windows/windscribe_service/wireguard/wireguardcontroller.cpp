// Have to include wireguard.h first or we'll get include conflicts in the Windows headers.
#include "wireguard.h"
#include "wireguardcontroller.h"

#include <Cfgmgr32.h>
#include <devguid.h>
#include <ndisguid.h>
#include <SetupAPI.h>

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>

#include "../../../client/common/types/global_consts.h"
#include "../../../client/common/utils/servicecontrolmanager.h"
#include "../../../client/common/utils/win32handle.h"
#include "../../../client/common/utils/wsscopeguard.h"
#include "../../../common/helper_commands.h"
#include "../executecmd.h"
#include "../utils.h"

#if defined(USE_SIGNATURE_CHECK)
#include "utils/executable_signature/executable_signature.h"
#endif

static const DEVPROPKEY WG_DEVP_KEYNAME = DEVPKEY_WG_NAME;


WireGuardController::WireGuardController()
{
}

bool WireGuardController::installService()
{
    is_initialized_ = false;
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

        is_initialized_ = true;
    }
    catch (std::system_error& ex) {
        spdlog::error("WireGuardController::installService - {}", ex.what());
    }

    return is_initialized_;
}

bool WireGuardController::deleteService()
{
    is_initialized_ = false;

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

    try {
        if (!is_initialized_) {
            throw std::system_error(ERROR_INVALID_STATE, std::generic_category(),
                "WireGuardController::getStatus - the WireGuard tunnel is not initialized");
        }

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
        else {
            lastHandshake = 0;
            txBytes = 0;
            rxBytes = 0;
        }
    }
    catch (std::system_error& ex) {
        result = kWgStateError;
        spdlog::error("WireGuardController::getStatus - {}", ex.what());
    }

    return result;
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
