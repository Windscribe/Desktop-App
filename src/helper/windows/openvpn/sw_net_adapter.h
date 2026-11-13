#pragma once

#include <winsock2.h>
#include <windows.h>
#include <setupapi.h>
#include <swdevice.h>

#include <memory>
#include <string>

#include "utils/systemlibloader.h"

// A network adapter created as a PnP software device (SwDeviceCreate), the way wireguard-nt and
// Wintun create theirs. Windows brings up a software device far faster than a root-enumerated one
// created through SetupAPI's class installer, which is what tapctl does: ~80 ms versus ~380 ms for
// ovpn-dco.
//
// The device outlives the helper process (SWDeviceLifetimeParentPresent), as the root-enumerated
// tapctl adapter did. If the helper crashes or is killed, a running OpenVPN keeps its device
// instead of spinning on device errors. remove() takes the device down, and removeOrphans()
// clears any device a dead helper left behind.
//
// Not thread-safe. The helper runs its commands on one thread.
class SwNetAdapter final
{
public:
    // enumerator becomes the middle part of the device instance path, SWD\<enumerator>\<id>, and
    // is how removeOrphans() finds this class's devices. It must not contain a backslash.
    SwNetAdapter(const std::wstring &enumerator, const std::wstring &hwid, const std::wstring &description);
    ~SwNetAdapter();

    SwNetAdapter(const SwNetAdapter &) = delete;
    SwNetAdapter &operator=(const SwNetAdapter &) = delete;

    // Creates the device, waits until its network interface is up, and sets the connection name
    // (the interface alias) to name. On failure nothing is left behind.
    bool create(const std::wstring &name);

    // Removes the device and its devnode. Does nothing if create() has not succeeded.
    void remove();

    bool isCreated() const { return device_ != nullptr; }

    // Removes every devnode under this enumerator, present or not. Only call it while this object
    // holds no device, since it would remove that one too.
    void removeOrphans();

private:
    using SwDeviceCreateFn = HRESULT WINAPI(PCWSTR, PCWSTR, const SW_DEVICE_CREATE_INFO *, ULONG, const DEVPROPERTY *,
                                            SW_DEVICE_CREATE_CALLBACK, PVOID, PHSWDEVICE);
    using SwDeviceCloseFn = void WINAPI(HSWDEVICE);
    using SwDeviceSetLifetimeFn = HRESULT WINAPI(HSWDEVICE, SW_DEVICE_LIFETIME);

    std::wstring enumerator_;
    std::wstring hwid_;
    std::wstring description_;

    // SwDevice* are exported only through onecore.lib, which would also rebind the helper's other
    // imports, so they are loaded at runtime. The DLL stays loaded while device_ is open: the
    // handle and its creation callback live in it.
    std::unique_ptr<wsl::SystemLibLoader> cfgmgr32_;
    SwDeviceCreateFn *swDeviceCreate_ = nullptr;
    SwDeviceCloseFn *swDeviceClose_ = nullptr;
    SwDeviceSetLifetimeFn *swDeviceSetLifetime_ = nullptr;

    HSWDEVICE device_ = nullptr;
    HDEVINFO devInfo_ = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA devInfoData_ = {};

    // Loads cfgmgr32.dll and resolves the SwDevice functions, once.
    bool loadCfgmgr32();

    // Calls SwDeviceCreate for SWD\<enumerator>\<instanceIdSuffix> and waits for the result. For
    // an existing device with no open handle, this reclaims it. Returns the handle and sets
    // instanceId to the full device instance ID, or returns nullptr.
    HSWDEVICE openDevice(const std::wstring &instanceIdSuffix, std::wstring &instanceId);

    // Returns the device to handle lifetime and closes the handle, which unplugs it.
    void closeDevice(HSWDEVICE device);

    // Waits for the interface the driver registers, then names it. instanceId is the full device
    // instance ID.
    bool finishCreate(const std::wstring &instanceId, const std::wstring &name);
};
