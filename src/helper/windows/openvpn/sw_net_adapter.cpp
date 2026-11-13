#include "sw_net_adapter.h"

#include <cfgmgr32.h>
#include <devguid.h>
#include <objbase.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

#include <memory>

#include <spdlog/spdlog.h>

#include "utils/log/debug_trace.h"

namespace {

// GUID_DEVINTERFACE_NET from ndisguid.h, spelled out so this file needs no INITGUID.
constexpr GUID kGuidDevInterfaceNet = {0xcac88484, 0x7515, 0x4c03, {0x82, 0xe6, 0x71, 0xa8, 0x7a, 0xba, 0xc3, 0x61}};

// SwDeviceCreate can report success to its callback before the driver is installed, so the
// interface is waited for separately. A first install of a new driver package can be slow, hence
// the generous limits. Wintun waits 15 s for the interface.
constexpr DWORD kCreateCallbackTimeoutMs = 30000;
constexpr ULONGLONG kInterfaceTimeoutMs = 15000;

// nci.dll has no import library.
using NciSetConnectionNameFn = DWORD WINAPI(const GUID *, LPCWSTR);

struct CreateContext
{
    HANDLE event = nullptr;
    HRESULT result = E_FAIL;
    wchar_t instanceId[MAX_DEVICE_ID_LEN] = {};
};

void WINAPI onDeviceCreated(HSWDEVICE, HRESULT createResult, PVOID context, PCWSTR instanceId)
{
    auto *ctx = static_cast<CreateContext *>(context);
    ctx->result = createResult;
    if (instanceId != nullptr) {
        wcsncpy_s(ctx->instanceId, instanceId, _TRUNCATE);
    }
    ::SetEvent(ctx->event);
}

std::wstring guidToString(const GUID &guid)
{
    wchar_t buf[64] = {};
    ::StringFromGUID2(guid, buf, _countof(buf));
    return buf;
}

// The net class installer writes NetCfgInstanceId into the driver key once the driver is
// installed. It is the interface GUID that NCI and the IP Helper API work with.
bool readNetCfgInstanceId(DEVINST devInst, GUID &guid)
{
    HKEY key = nullptr;
    if (::CM_Open_DevNode_Key(devInst, KEY_QUERY_VALUE, 0, RegDisposition_OpenExisting, &key, CM_REGISTRY_SOFTWARE) != CR_SUCCESS) {
        return false;
    }
    wchar_t buf[64] = {};
    DWORD size = sizeof(buf);
    const LSTATUS status = ::RegGetValueW(key, nullptr, L"NetCfgInstanceId", RRF_RT_REG_SZ, nullptr, buf, &size);
    ::RegCloseKey(key);
    return status == ERROR_SUCCESS && SUCCEEDED(::CLSIDFromString(buf, &guid));
}

// OpenVPN opens the adapter through this device interface, so wait for exactly that.
bool isNetDeviceInterfacePresent(const std::wstring &instanceId)
{
    ULONG size = 0;
    const CONFIGRET ret = ::CM_Get_Device_Interface_List_SizeW(&size, const_cast<GUID *>(&kGuidDevInterfaceNet),
                                                               const_cast<wchar_t *>(instanceId.c_str()),
                                                               CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    // An empty list is a lone terminator.
    return ret == CR_SUCCESS && size > 1;
}

bool removeDevice(HDEVINFO devInfo, SP_DEVINFO_DATA *devInfoData, const std::wstring &instanceId)
{
    SP_REMOVEDEVICE_PARAMS params = {};
    params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    params.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    params.Scope = DI_REMOVEDEVICE_GLOBAL;
    params.HwProfile = 0;
    if (!::SetupDiSetClassInstallParamsW(devInfo, devInfoData, &params.ClassInstallHeader, sizeof(params)) ||
        !::SetupDiCallClassInstaller(DIF_REMOVE, devInfo, devInfoData)) {
        spdlog::error(L"SwNetAdapter: failed to remove device {}: {}", instanceId, ::GetLastError());
        return false;
    }

    SP_DEVINSTALL_PARAMS_W installParams = {};
    installParams.cbSize = sizeof(installParams);
    if (::SetupDiGetDeviceInstallParamsW(devInfo, devInfoData, &installParams) &&
        (installParams.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART))) {
        spdlog::warn(L"SwNetAdapter: removed device {}, but a reboot is required to complete it", instanceId);
    }
    return true;
}

} // namespace

SwNetAdapter::SwNetAdapter(const std::wstring &enumerator, const std::wstring &hwid, const std::wstring &description)
    : enumerator_(enumerator), hwid_(hwid), description_(description)
{
}

SwNetAdapter::~SwNetAdapter()
{
    remove();
}

bool SwNetAdapter::loadCfgmgr32()
{
    if (cfgmgr32_) {
        return true;
    }
    try {
        auto lib = std::make_unique<wsl::SystemLibLoader>("cfgmgr32.dll");
        swDeviceCreate_ = lib->getFunction<SwDeviceCreateFn>("SwDeviceCreate");
        swDeviceClose_ = lib->getFunction<SwDeviceCloseFn>("SwDeviceClose");
        swDeviceSetLifetime_ = lib->getFunction<SwDeviceSetLifetimeFn>("SwDeviceSetLifetime");
        cfgmgr32_ = std::move(lib);
        return true;
    } catch (const std::exception &ex) {
        spdlog::error("SwNetAdapter: {}", ex.what());
        return false;
    }
}

HSWDEVICE SwNetAdapter::openDevice(const std::wstring &instanceIdSuffix, std::wstring &instanceId)
{
    DEVINST root = 0;
    wchar_t rootId[MAX_DEVICE_ID_LEN] = {};
    CONFIGRET cr = ::CM_Locate_DevNodeW(&root, nullptr, CM_LOCATE_DEVNODE_NORMAL);
    if (cr == CR_SUCCESS) {
        cr = ::CM_Get_Device_IDW(root, rootId, _countof(rootId), 0);
    }
    if (cr != CR_SUCCESS) {
        spdlog::error("SwNetAdapter: could not get the root device ID: {}", cr);
        return nullptr;
    }

    // Hardware IDs are a multi-string: the extra terminator plus c_str()'s ends the list.
    std::wstring hardwareIds = hwid_;
    hardwareIds.push_back(L'\0');

    SW_DEVICE_CREATE_INFO createInfo = {};
    createInfo.cbSize = sizeof(createInfo);
    createInfo.pszInstanceId = instanceIdSuffix.c_str();
    createInfo.pszzHardwareIds = hardwareIds.c_str();
    createInfo.CapabilityFlags = SWDeviceCapabilitiesSilentInstall | SWDeviceCapabilitiesDriverRequired;
    createInfo.pszDeviceDescription = description_.c_str();

    CreateContext ctx;
    ctx.event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (ctx.event == nullptr) {
        spdlog::error("SwNetAdapter: CreateEvent failed: {}", ::GetLastError());
        return nullptr;
    }

    HSWDEVICE device = nullptr;
    const HRESULT hr = swDeviceCreate_(enumerator_.c_str(), rootId, &createInfo, 0, nullptr, onDeviceCreated, &ctx, &device);
    if (FAILED(hr)) {
        spdlog::error("SwNetAdapter: SwDeviceCreate failed: {:#010x}", static_cast<unsigned long>(hr));
        ::CloseHandle(ctx.event);
        return nullptr;
    }

    const DWORD waitResult = ::WaitForSingleObject(ctx.event, kCreateCallbackTimeoutMs);
    if (waitResult != WAIT_OBJECT_0) {
        spdlog::error("SwNetAdapter: timed out waiting for SwDeviceCreate (wait result {})", waitResult);
        // Once SwDeviceClose returns, the callback is guaranteed not to run, so ctx can go.
        swDeviceClose_(device);
        ::CloseHandle(ctx.event);
        return nullptr;
    }
    ::CloseHandle(ctx.event);

    if (FAILED(ctx.result)) {
        spdlog::error("SwNetAdapter: device creation failed: {:#010x}", static_cast<unsigned long>(ctx.result));
        swDeviceClose_(device);
        return nullptr;
    }

    instanceId = ctx.instanceId;
    return device;
}

void SwNetAdapter::closeDevice(HSWDEVICE device)
{
    // Back to handle lifetime first, as Microsoft recommends before uninstalling, so that
    // SwDeviceClose unplugs the device.
    const HRESULT hr = swDeviceSetLifetime_(device, SWDeviceLifetimeHandle);
    if (FAILED(hr)) {
        // The caller's DIF_REMOVE still removes the devnode.
        spdlog::warn("SwNetAdapter: SwDeviceSetLifetime(handle) failed: {:#010x}", static_cast<unsigned long>(hr));
    }
    swDeviceClose_(device);
}

bool SwNetAdapter::create(const std::wstring &name)
{
    WS_DEBUG_TRACE_SCOPE("SwNetAdapter::create");
    remove();

    if (!loadCfgmgr32()) {
        return false;
    }

    // A fresh instance ID each time, so a devnode a failed removal left behind can never be
    // reused with stale state.
    GUID instanceGuid;
    if (FAILED(::CoCreateGuid(&instanceGuid))) {
        spdlog::error("SwNetAdapter: CoCreateGuid failed");
        return false;
    }

    std::wstring instanceId;
    device_ = openDevice(guidToString(instanceGuid), instanceId);
    if (device_ == nullptr) {
        return false;
    }

    // Only allowed once the creation callback has run, which openDevice waited for. A failure
    // here is fatal: the caller falls back to tapctl, which keeps the crash behavior described in
    // the header.
    const HRESULT hr = swDeviceSetLifetime_(device_, SWDeviceLifetimeParentPresent);
    if (FAILED(hr)) {
        spdlog::error("SwNetAdapter: SwDeviceSetLifetime failed: {:#010x}", static_cast<unsigned long>(hr));
        remove();
        return false;
    }

    spdlog::info(L"SwNetAdapter: created device {}", instanceId);

    // Opened now so remove() can still DIF_REMOVE the devnode once SwDeviceClose has taken it
    // off line.
    devInfo_ = ::SetupDiCreateDeviceInfoListExW(nullptr, nullptr, nullptr, nullptr);
    if (devInfo_ == INVALID_HANDLE_VALUE) {
        spdlog::error("SwNetAdapter: SetupDiCreateDeviceInfoListEx failed: {}", ::GetLastError());
        remove();
        return false;
    }
    devInfoData_ = {};
    devInfoData_.cbSize = sizeof(devInfoData_);
    if (!::SetupDiOpenDeviceInfoW(devInfo_, instanceId.c_str(), nullptr, 0, &devInfoData_)) {
        spdlog::error(L"SwNetAdapter: SetupDiOpenDeviceInfo({}) failed: {}", instanceId, ::GetLastError());
        ::SetupDiDestroyDeviceInfoList(devInfo_);
        devInfo_ = INVALID_HANDLE_VALUE;
        remove();
        return false;
    }

    if (!finishCreate(instanceId, name)) {
        remove();
        return false;
    }
    return true;
}

bool SwNetAdapter::finishCreate(const std::wstring &instanceId, const std::wstring &name)
{
    const DEVINST devInst = devInfoData_.DevInst;

    GUID interfaceGuid = {};
    NET_LUID luid = {};
    const ULONGLONG deadline = ::GetTickCount64() + kInterfaceTimeoutMs;
    {
        WS_DEBUG_TRACE_SCOPE("SwNetAdapter::create wait for interface");
        for (;;) {
            if (readNetCfgInstanceId(devInst, interfaceGuid) && isNetDeviceInterfacePresent(instanceId) &&
                ::ConvertInterfaceGuidToLuid(&interfaceGuid, &luid) == NO_ERROR) {
                break;
            }
            if (::GetTickCount64() >= deadline) {
                ULONG status = 0;
                ULONG problem = 0;
                const CONFIGRET cr = ::CM_Get_DevNode_Status(&status, &problem, devInst, 0);
                spdlog::error(L"SwNetAdapter: interface for {} did not come up (CM_Get_DevNode_Status {}, status {:#x}, problem {})",
                              instanceId, cr, status, problem);
                return false;
            }
            ::Sleep(1);
        }
    }

    DWORD renameResult = ERROR_SUCCESS;
    try {
        wsl::SystemLibLoader nci("nci.dll");
        renameResult = nci.getFunction<NciSetConnectionNameFn>("NciSetConnectionName")(&interfaceGuid, name.c_str());
    } catch (const std::exception &ex) {
        spdlog::error("SwNetAdapter: {}", ex.what());
        return false;
    }
    if (renameResult != ERROR_SUCCESS) {
        spdlog::error(L"SwNetAdapter: NciSetConnectionName({}, {}) failed: {}", guidToString(interfaceGuid), name, renameResult);
        return false;
    }

    // OpenVPN's --dev-node and the helper's DAD fix look the adapter up by this name.
    NET_LUID aliasLuid = {};
    const DWORD aliasResult = ::ConvertInterfaceAliasToLuid(name.c_str(), &aliasLuid);
    if (aliasResult != NO_ERROR || aliasLuid.Value != luid.Value) {
        spdlog::warn(L"SwNetAdapter: the name {} does not resolve to the new interface yet (result {})", name, aliasResult);
    }

    spdlog::info(L"SwNetAdapter: adapter {} is up, interface {}", name, guidToString(interfaceGuid));
    return true;
}

void SwNetAdapter::remove()
{
    if (device_ == nullptr && devInfo_ == INVALID_HANDLE_VALUE) {
        return;
    }
    WS_DEBUG_TRACE_SCOPE("SwNetAdapter::remove");

    if (device_ != nullptr) {
        // Its devnode, driver key and per-interface TCP/IP settings stay until DIF_REMOVE below,
        // as Wintun does.
        closeDevice(device_);
        device_ = nullptr;
    }

    if (devInfo_ != INVALID_HANDLE_VALUE) {
        wchar_t instanceId[MAX_DEVICE_ID_LEN] = L"<unknown>";
        ::SetupDiGetDeviceInstanceIdW(devInfo_, &devInfoData_, instanceId, _countof(instanceId), nullptr);
        if (removeDevice(devInfo_, &devInfoData_, instanceId)) {
            spdlog::info(L"SwNetAdapter: removed device {}", instanceId);
        }
        ::SetupDiDestroyDeviceInfoList(devInfo_);
        devInfo_ = INVALID_HANDLE_VALUE;
    }
}

void SwNetAdapter::removeOrphans()
{
    WS_DEBUG_TRACE_SCOPE("SwNetAdapter::removeOrphans");

    // Flags without DIGCF_PRESENT, so non-present devnodes are listed too. All classes, since a
    // device whose driver never installed may not be in the Net class.
    const std::wstring enumeratorPath = L"SWD\\" + enumerator_;
    HDEVINFO devInfo = ::SetupDiGetClassDevsExW(nullptr, enumeratorPath.c_str(), nullptr, DIGCF_ALLCLASSES, nullptr, nullptr, nullptr);
    if (devInfo == INVALID_HANDLE_VALUE) {
        spdlog::error(L"SwNetAdapter: SetupDiGetClassDevsEx({}) failed: {}", enumeratorPath, ::GetLastError());
        return;
    }

    for (DWORD index = 0;; ++index) {
        SP_DEVINFO_DATA devInfoData = {};
        devInfoData.cbSize = sizeof(devInfoData);
        if (!::SetupDiEnumDeviceInfo(devInfo, index, &devInfoData)) {
            if (::GetLastError() == ERROR_NO_MORE_ITEMS) {
                break;
            }
            continue;
        }
        wchar_t instanceId[MAX_DEVICE_ID_LEN] = L"<unknown>";
        ::SetupDiGetDeviceInstanceIdW(devInfo, &devInfoData, instanceId, _countof(instanceId), nullptr);

        // A leftover device still has parent-present lifetime. DIF_REMOVE alone reports success
        // but leaves it present. SwDeviceCreate with the same instance ID reclaims a device no one
        // holds a handle to, which lets closeDevice() unplug it before the devnode is removed.
        const std::wstring fullId = instanceId;
        const size_t separator = fullId.rfind(L'\\');
        if (separator != std::wstring::npos && loadCfgmgr32()) {
            std::wstring reclaimedId;
            if (HSWDEVICE device = openDevice(fullId.substr(separator + 1), reclaimedId)) {
                closeDevice(device);
            }
        }

        if (removeDevice(devInfo, &devInfoData, instanceId)) {
            spdlog::info(L"SwNetAdapter: removed orphaned device {}", instanceId);
        }
    }
    ::SetupDiDestroyDeviceInfoList(devInfo);
}
