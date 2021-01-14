#include "../all_headers.h"
#include "../executecmd.h"
#include "../ip_address/ip4_address_and_mask.h"
#include "../logger.h"
#include "../utils.h"
#include "wireguardadapter.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace
{
std::wstring GetDeviceGuid(const std::wstring &deviceName)
{
    const std::wstring kNetworkConnectionsKeyName(
        L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
    HKEY connectionsKey;
    auto status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, kNetworkConnectionsKeyName.c_str(), 0,
        KEY_READ, &connectionsKey);
    if (status != NO_ERROR)
        return std::wstring();
    std::wstring strGuid;
    wchar_t keyname[256] = {};
    for (DWORD index = 0;; ++index) {
        DWORD keysize = sizeof(keyname);
        status = RegEnumKeyEx(connectionsKey, index, keyname, &keysize, nullptr, nullptr, nullptr,
            nullptr);
        if (status != NO_ERROR)
            break;
        std::wstring subkeyName(keyname);
        subkeyName.append(L"\\Connection");
        HKEY connectionsSubkey;
        status = RegOpenKeyEx(connectionsKey, subkeyName.c_str(), 0, KEY_READ, &connectionsSubkey);
        if (status != NO_ERROR)
            continue;
        wchar_t namebuffer[256] = {};
        DWORD namebuffersize = sizeof(namebuffer);
        status = RegQueryValueEx(connectionsSubkey, L"Name", 0, nullptr,
            reinterpret_cast<LPBYTE>(namebuffer), &namebuffersize);
        RegCloseKey(connectionsSubkey);
        if (status != NO_ERROR)
            continue;
        if (!deviceName.compare(namebuffer)) {
            strGuid = keyname;
            break;
        }
    }
    RegCloseKey(connectionsKey);
    return strGuid;
}

void CleanupAddressOnDisconnectedInterfaces(const Ip4AddressAndMask &address, UINT8 cidr)
{
    const int kMaxTries = 5;
    ULONG status = NO_ERROR, bufferSize = 15000u;
    std::vector<BYTE> buffer;
    buffer.resize(bufferSize);
    for (int i = 0; i < kMaxTries; ++i) {
        status = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr,
            reinterpret_cast<PIP_ADAPTER_ADDRESSES>(*buffer.begin()), &bufferSize);
        if (status == ERROR_BUFFER_OVERFLOW)
            buffer.resize(bufferSize);
        else
            break;
    }
    if (status != NO_ERROR)
        return;
    PIP_ADAPTER_ADDRESSES data = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    for (; data; data = data->Next) {
        if (data->OperStatus == IfOperStatusUp)
            continue;
        for (const auto *addr = data->FirstUnicastAddress; addr; addr = addr->Next) {
            if (addr->Address.lpSockaddr->sa_family != AF_INET ||
                addr->OnLinkPrefixLength != cidr)
                continue;
            const auto *ip4addr = reinterpret_cast<const sockaddr_in*>(addr->Address.lpSockaddr);
            if (ip4addr->sin_addr.S_un.S_addr != address.ipNetworkOrder())
                continue;
            Logger::instance().out(L"Cleaning up stale address %ls from interface %ls",
                address.asString().c_str(), data->FriendlyName);
            MIB_UNICASTIPADDRESS_ROW ipentry;
            InitializeUnicastIpAddressEntry(&ipentry);
            ipentry.InterfaceLuid = data->Luid;
            ipentry.Address.Ipv4.sin_family = AF_INET;
            ipentry.Address.Ipv4.sin_addr.S_un.S_addr = address.ipNetworkOrder();
            status = DeleteUnicastIpAddressEntry(&ipentry);
            if (status != NO_ERROR)
                Logger::instance().out(L"DeleteUnicastIpAddressEntry failed");
        }
    }
}

bool RunBlockingCommands(const std::vector<std::wstring> &cmdlist)
{
    MessagePacketResult mpr;
    mpr.success = true;
    wchar_t command_buffer[MAX_PATH];
    for (const auto &cmd : cmdlist) {
        memset(command_buffer, 0, sizeof(command_buffer));
        wcscat_s(command_buffer, cmd.c_str());
        mpr = ExecuteCmd::instance().executeBlockingCmd(command_buffer);
        if (!mpr.success) {
            Logger::instance().out(L"Command failed: %ls", command_buffer);
            if (mpr.sizeOfAdditionalData)
                Logger::instance().out(L"Output: %ls", mpr.szAdditionalData);
        }
        mpr.clear();
    }
    return mpr.success;
}
} // namespace

WireGuardAdapter::WireGuardAdapter(const std::wstring &name)
    : name_(name), is_adapter_initialized_(false), is_routing_enabled_(false)
{
}

WireGuardAdapter::~WireGuardAdapter()
{
    disableRouting();
    flushDnsServer();
    unsetIpAddress();
}

void WireGuardAdapter::initializeOnce()
{
    if (is_adapter_initialized_)
        return;
    guidString_ = GetDeviceGuid(name_);
    if (guidString_.empty())
        return;
    guid_ = Utils::guidFromString(guidString_);
    ConvertInterfaceGuidToLuid(&guid_, &luid_);
    Logger::instance().out(L"WireGuardAdapter GUID: \"%ls\"", guidString_.c_str());
    is_adapter_initialized_ = true;
}

bool WireGuardAdapter::setIpAddress(const std::string &address, bool isDefault)
{
    initializeOnce();
    if (!unsetIpAddress())
        return false;
    std::vector<std::string> address_and_cidr;
    boost::split(address_and_cidr, address, boost::is_any_of("/"), boost::token_compress_on);
    Ip4AddressAndMask ipaddr(address_and_cidr[0].c_str());
    UINT8 cidr = 0;
    if (address_and_cidr.size() > 1)
        cidr = strtol(address_and_cidr[1].c_str(), nullptr, 10);
    CleanupAddressOnDisconnectedInterfaces(ipaddr, cidr);

    MIB_IPINTERFACE_ROW entry;
    InitializeIpInterfaceEntry(&entry);
    entry.InterfaceLuid = luid_;
    entry.Family = AF_INET;
    auto status = GetIpInterfaceEntry(&entry);
    if (status != NO_ERROR) {
        Logger::instance().out(L"GetIpInterfaceEntry failed");
        return false;
    }
    if (entry.SitePrefixLength > 32)
        entry.SitePrefixLength = 0;
    if (isDefault) {
        entry.UseAutomaticMetric = FALSE;
        entry.Metric = 0;
    }
    entry.NlMtu = 1420;  // This should be updated later by the engine.
    status = SetIpInterfaceEntry(&entry);
    if (status != NO_ERROR) {
        Logger::instance().out(L"SetIpInterfaceEntry failed");
        return false;
    }
    MIB_UNICASTIPADDRESS_ROW ipentry;
    InitializeUnicastIpAddressEntry(&ipentry);
    ipentry.InterfaceLuid = luid_;
    ipentry.Address.Ipv4.sin_family = AF_INET;
	ipentry.Address.Ipv4.sin_addr.S_un.S_addr = ipaddr.ipNetworkOrder();
    ipentry.OnLinkPrefixLength = cidr;
    status = CreateUnicastIpAddressEntry(&ipentry);
    if (status != NO_ERROR) {
        Logger::instance().out(L"CreateUnicastIpAddressEntry failed");
        return false;
    }
    return true;
}

bool WireGuardAdapter::setDnsServers(const std::string &addressList)
{
    initializeOnce();
    if (!clearDnsDomain())
        return false;
    std::vector<std::string> dns_addresses;
    boost::split(dns_addresses, addressList, boost::is_any_of(",; "), boost::token_compress_on);
    if (dns_addresses.size() < 1) {
        Logger::instance().out(L"Invalid DNS address list: %s", addressList.c_str());
        return false;
    }
    std::vector<std::wstring> cmdlist;
    MIB_IPINTERFACE_ROW entry;
    InitializeIpInterfaceEntry(&entry);
    entry.InterfaceLuid = luid_;
    entry.Family = AF_INET;
    auto status = GetIpInterfaceEntry(&entry);
    if (status != NO_ERROR) {
        Logger::instance().out(L"GetIpInterfaceEntry failed");
        return false;
    }
    for (const auto &dns : dns_addresses) {
        cmdlist.push_back(L"netsh int ipv4 add dnsservers name="
            + std::to_wstring(entry.InterfaceIndex)
            + L" address="
            + std::wstring(dns.begin(), dns.end())
            + L" validate=no");
    }
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps)
{
    initializeOnce();
    disableRouting();

    bool success = true;
    for (const auto &ip : allowedIps) {
        std::vector<std::string> address_and_cidr;
        boost::split(address_and_cidr, ip, boost::is_any_of("/"), boost::token_compress_on);
        Ip4AddressAndMask ipaddr(address_and_cidr[0].c_str());
        UINT8 cidr = 0;
        if (address_and_cidr.size() > 1)
            cidr = strtol(address_and_cidr[1].c_str(), nullptr, 10);
        MIB_IPFORWARD_ROW2 entry;
        InitializeIpForwardEntry(&entry);
        entry.InterfaceLuid = luid_;
        entry.DestinationPrefix.Prefix.si_family = AF_INET;
		entry.DestinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_addr = ipaddr.ipNetworkOrder();
        entry.DestinationPrefix.PrefixLength = cidr;
        entry.NextHop.si_family = AF_INET;
        entry.NextHop.Ipv4.sin_addr.S_un.S_addr = 0;
        entry.Metric = 0;
        auto status = CreateIpForwardEntry2(&entry);
        if (status != NO_ERROR) {
            Logger::instance().out(L"Failed to add route: %s", ip.c_str());
            success = false;
            break;
        }
    }
    is_routing_enabled_ = true;
    return success;
}

bool WireGuardAdapter::unsetIpAddress()
{
    if (!is_adapter_initialized_)
        return true;
    MIB_UNICASTIPADDRESS_TABLE *table = nullptr;
    auto status = GetUnicastIpAddressTable(AF_UNSPEC, &table);
    if (status != NO_ERROR) {
        Logger::instance().out(L"GetUnicastIpAddressTable failed");
        return false;
    }
    for (DWORD i = 0; i < table->NumEntries; ++i) {
        const auto *row = &table->Table[i];
        if (row->InterfaceLuid.Value != luid_.Value)
            continue;
        status = DeleteUnicastIpAddressEntry(row);
        if (status != NO_ERROR)
            Logger::instance().out(L"DeleteUnicastIpAddressEntry failed");
    }
    FreeMibTable(table);
    return true;
}

bool WireGuardAdapter::flushDnsServer()
{
    if (!is_adapter_initialized_)
        return true;
    std::vector<std::wstring> cmdlist;
    MIB_IPINTERFACE_ROW entry;
    InitializeIpInterfaceEntry(&entry);
    entry.InterfaceLuid = luid_;
    entry.Family = AF_INET;
    auto status = GetIpInterfaceEntry(&entry);
    if (status == NO_ERROR) {
        cmdlist.push_back(L"netsh int ipv4 set dnsservers name="
            + std::to_wstring(entry.InterfaceIndex)
            + L" source=static address=none validate=no register=both");
    }
    InitializeIpInterfaceEntry(&entry);
    entry.InterfaceLuid = luid_;
    entry.Family = AF_INET6;
    status = GetIpInterfaceEntry(&entry);
    if (status == NO_ERROR) {
        cmdlist.push_back(L"netsh int ipv6 set dnsservers name="
            + std::to_wstring(entry.InterfaceIndex)
            + L" source=static address=none validate=no register=both");
    }
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::clearDnsDomain()
{
    if (!is_adapter_initialized_)
        return true;
    const std::wstring kTcpipParmsKeyPath(
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\");
    HKEY adapterKey;
    const auto kKeyName = kTcpipParmsKeyPath + guidString_;
    auto status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, kKeyName.c_str(), 0, KEY_QUERY_VALUE,
                               &adapterKey);
    if (status != NO_ERROR) {
        Logger::instance().out(L"Unable to open adapter-specific TCP/IP network registry key: %ls (error %u)",
            kKeyName.c_str(), (UINT)status);
        return false;
    }
    wchar_t namebuffer[256] = {};
    DWORD namebuffersize = sizeof(namebuffer);
    status = RegQueryValueEx(adapterKey, L"IpConfig", 0, nullptr,
        reinterpret_cast<LPBYTE>(namebuffer), &namebuffersize);
    if (status != NO_ERROR) {
        Logger::instance().out(L"No TCP/IP interfaces found on adapter");
        return false;
    }
    RegCloseKey(adapterKey);
    const std::wstring ipConfigPath(namebuffer);
    if (!ipConfigPath.empty()) {
        const auto kKeyName2 = L"SYSTEM\\CurrentControlSet\\Services\\" + ipConfigPath;
        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, kKeyName2.c_str(), 0, KEY_SET_VALUE, &adapterKey);
        if (status != NO_ERROR) {
            Logger::instance().out(L"Unable to open TCP/IP network registry key: %ls",
                kKeyName2.c_str());
            return false;
        }
        BYTE emptyDomain[] = "";
        status = RegSetValueEx(adapterKey, L"Domain", 0, REG_SZ, emptyDomain, sizeof(emptyDomain));
        RegCloseKey(adapterKey);
        if (status != NO_ERROR) {
            Logger::instance().out(L"Unable to set TCP/IP network registry key: %ls",
                kKeyName2.c_str());
            return false;
        }
    }
    return true;
}

bool WireGuardAdapter::disableRouting()
{
    if (!is_adapter_initialized_ || !is_routing_enabled_)
        return true;
    MIB_IPFORWARD_TABLE2 *table = nullptr;
    auto status = GetIpForwardTable2(AF_UNSPEC, &table);
    if (status != NO_ERROR) {
        Logger::instance().out(L"GetIpForwardTable2 failed");
        return false;
    }
    for (DWORD i = 0; i < table->NumEntries; ++i) {
        const auto *row = &table->Table[i];
        if (row->InterfaceLuid.Value != luid_.Value)
            continue;
        status = DeleteIpForwardEntry2(row);
        if (status != NO_ERROR)
            Logger::instance().out(L"DeleteIpForwardEntry2 failed");
    }
    FreeMibTable(table);
    return true;
}
