#include "network_utils_win.h"

#include <QList>

#include <Windows.h>

#include <WS2tcpip.h>
#include <atlbase.h>
#include <iphlpapi.h>
#include <netlistmgr.h>

#include "../log/categories.h"
#include "../networktypes.h"
#include "../winutils.h"
#include "wlan_utils_win.h"

static bool g_SsidAccessDenied = false;

static QString guidToQString(GUID guid)
{
    OLECHAR* guidString;
    StringFromCLSID(guid, &guidString);

    std::wstring str = (guidString);
    QString guidQString = QString::fromWCharArray(str.c_str());

    ::CoTaskMemFree(guidString);

    return guidQString;
}

static QList<QString> singleHexChars()
{
    return QList<QString>() << "0" << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9" << "a" << "b" << "c" << "d" << "e" << "f";
}

static QList<IfTableRow> getIfTable()
{
    QList<IfTableRow> table;

    // Make an initial call to GetIfTable to get the necessary size into dwSize
    DWORD dwSize = 0;
    auto result = GetIfTable(NULL, &dwSize, FALSE);
    if (result != ERROR_INSUFFICIENT_BUFFER) {
        qCDebug(LOG_BASIC) << "Initial GetIfTable unexpected failure:" << result;
        return table;
    }

    QScopedArrayPointer<unsigned char> buf(new unsigned char[dwSize]);
    PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)buf.data();

    // Make a second call to GetIfTable to get the actual data we want.
    result = GetIfTable(pIfTable, &dwSize, FALSE);
    if (result != NO_ERROR) {
        qCDebug(LOG_BASIC) << "GetIfTable failed with error:" << result;
        return table;
    }

    for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
        NETWORK_INTERFACE_TYPE nicType = NETWORK_INTERFACE_NONE;
        if (pIfTable->table[i].dwType == IF_TYPE_ETHERNET_CSMACD) {
            nicType = NETWORK_INTERFACE_ETH;
        }
        else if (pIfTable->table[i].dwType == IF_TYPE_IEEE80211) {
            nicType = NETWORK_INTERFACE_WIFI;
        }
        else if (pIfTable->table[i].dwType == IF_TYPE_PPP) {
            nicType = NETWORK_INTERFACE_PPP;
        }
        else if (pIfTable->table[i].dwType == IF_TYPE_IEEE80216_WMAN || pIfTable->table[i].dwType == IF_TYPE_WWANPP || pIfTable->table[i].dwType == IF_TYPE_WWANPP2) {
            nicType = NETWORK_INTERFACE_MOBILE_BROADBAND;
        }

        // qDebug() << "Index: " << pIfTable->table[i].dwIndex << ", State: " << pIfTable->table[i].dwOperStatus;

        QString interfaceName = "";
        for (DWORD j = 0; j < pIfTable->table[i].dwDescrLen; j++) {
            interfaceName.append(static_cast<char>(pIfTable->table[i].bDescr[j]));
        }

        QString physicalAddress = "";
        for (DWORD j = 0; j < pIfTable->table[i].dwPhysAddrLen; j++) {
            physicalAddress.append(static_cast<char>(pIfTable->table[i].bPhysAddr[j]));
        }

        IfTableRow ifRow(static_cast<int>(pIfTable->table[i].dwIndex),
                         QString::fromWCharArray(pIfTable->table[i].wszName),
                         interfaceName,
                         physicalAddress,
                         nicType,
                         static_cast<int>(pIfTable->table[i].dwType),
                         static_cast<int>(pIfTable->table[i].dwMtu),
                         pIfTable->table[i].dwOperStatus >= IF_OPER_STATUS_CONNECTED,
                         (int) pIfTable->table[i].dwOperStatus);
        table.append(ifRow);
    }

    return table;
}

static QList<IfTable2Row> getIfTable2()
{
    QList<IfTable2Row> if2Table;

    PMIB_IF_TABLE2 pIfTable2;
    auto result = GetIfTable2(&pIfTable2);
    if (result != NO_ERROR) {
        qCDebug(LOG_BASIC) << "GetIfTable2 failed:" << result;
        return if2Table;
    }

    for (ULONG i = 0; i < pIfTable2->NumEntries; i++) {
        PMIB_IF_ROW2 pEntry = &pIfTable2->Table[i];
        if (!pEntry->InterfaceAndOperStatusFlags.FilterInterface) {
            QString guid = guidToQString(pEntry->InterfaceGuid);
            QString description = QString::fromWCharArray(pEntry->Description);
            QString alias = QString::fromWCharArray(pEntry->Alias);

            IfTable2Row row(pEntry->InterfaceIndex,
                            guid,
                            description,
                            alias,
                            pEntry->AccessType,
                            pEntry->InterfaceAndOperStatusFlags.ConnectorPresent,
                            pEntry->InterfaceAndOperStatusFlags.EndPointInterface,
                            pEntry->Type);
            if2Table.append(row);
        }
    }

    FreeMibTable(pIfTable2);

    return if2Table;
}

static IfTableRow ifRowByIndex(int index)
{
    IfTableRow found;

    const auto if_table = getIfTable();
    for (const IfTableRow &row : if_table) {
        if (index == static_cast<int>(row.index)) { // TODO: convert all ifIndices to int
            found = row;
            break;
        }
    }

    return found;
}

static QString interfaceSubkeyPath(int interfaceIndex)
{
    IfTableRow row = ifRowByIndex(interfaceIndex);

    const QString keyPath("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
    const QStringList ifSubkeys = WinUtils::enumerateSubkeyNames(HKEY_LOCAL_MACHINE, keyPath);

    QString foundSubkey = "";
    for (const QString &subkey : ifSubkeys) {
        QString subkeyPath = keyPath + "\\" + subkey;
        QString subkeyInterfaceGuid = WinUtils::regGetLocalMachineRegistryValueSz(subkeyPath, "NetCfgInstanceId");

        if (subkeyInterfaceGuid == row.guidName) {
            foundSubkey = subkeyPath; // whole key path
            break;
        }
    }

    return foundSubkey;
}

static bool interfaceSubkeyHasProperty(int interfaceIndex, QString propertyName)
{
    QString interfaceSubkeyP = interfaceSubkeyPath(interfaceIndex);
    return WinUtils::regHasLocalMachineSubkeyProperty(interfaceSubkeyP, propertyName);
}

static IfTable2Row ifTable2RowByIndex(int index)
{
    IfTable2Row found;

    const auto if_table = getIfTable2();
    for (const IfTable2Row &row : if_table) {
        if (index == static_cast<int>(row.index)) { // TODO: convert all ifIndices to int
            found = row;
            break;
        }
    }

    return found;
}

static QList<IpAdapter> getIpAdapterTable()
{
    QList<IpAdapter> adapters;

    ULONG bufSize = sizeof(IP_ADAPTER_INFO) * 32;
    QByteArray pAdapterInfo(bufSize, Qt::Uninitialized);

    DWORD result = ::GetAdaptersInfo((IP_ADAPTER_INFO*)pAdapterInfo.data(), &bufSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo.resize(bufSize);
        result = ::GetAdaptersInfo((IP_ADAPTER_INFO*)pAdapterInfo.data(), &bufSize);
    }

    if (result != NO_ERROR) {
        if (result != ERROR_NO_DATA) {
            qCDebug(LOG_BASIC) << "getIpAdapterTable: GetAdaptersInfo failed" << result;
        }
        return adapters;
    }

    IP_ADAPTER_INFO *pAdapter = (IP_ADAPTER_INFO*)pAdapterInfo.data();
    while (pAdapter) {
        QString physAddress = "";
        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
            QString s = QString("%1").arg(pAdapter->Address[i], 0, 16);

            if (singleHexChars().contains(s)) {
                s = "0" + s;
            }
            physAddress += s;
        }

        IpAdapter ipAdapter(pAdapter->Index, pAdapter->AdapterName, pAdapter->Description, physAddress);
        adapters.append(ipAdapter);

        pAdapter = pAdapter->Next;
    }

    return adapters;
}

static QList<IpForwardRow> getIpForwardTable()
{
    QList<IpForwardRow> forwardTable;

    DWORD dwSize = 0;
    auto result = GetIpForwardTable(NULL, &dwSize, 0);
    if (result != ERROR_INSUFFICIENT_BUFFER) {
        if (result != ERROR_NO_DATA) {
            qCDebug(LOG_BASIC) << "Initial GetIpForwardTable unexpected failure:" << result;
        }
        return forwardTable;
    }

    QScopedArrayPointer<unsigned char> buf(new unsigned char[dwSize]);
    PMIB_IPFORWARDTABLE pIpForwardTable = (PMIB_IPFORWARDTABLE)buf.data();

    /* Note that the IPv4 addresses returned in
     * GetIpForwardTable entries are in network byte order
     */
    result = GetIpForwardTable(pIpForwardTable, &dwSize, 0);
    if (result != NO_ERROR) {
        qCDebug(LOG_BASIC) << "GetIpForwardTable failed:" << result;
        return forwardTable;
    }

    for (auto i = 0; i < pIpForwardTable->dwNumEntries; i++) {
        IpForwardRow ipRow = IpForwardRow(pIpForwardTable->table[i].dwForwardIfIndex,
                                          pIpForwardTable->table[i].dwForwardMetric1);
        forwardTable.append(ipRow);
    }

    return forwardTable;
}

static IfTable2Row lowestMetricNonWindscribeIfTableRow()
{
    IfTable2Row lowestMetricIfRow;

    const QList<IpForwardRow> fwdTable = getIpForwardTable();
    const QList<IpAdapter> ipAdapters = getIpAdapterTable();

    int lowestMetric = 999999;

    for (const IpForwardRow &row: fwdTable) {
        for (const IpAdapter &ipAdapter: ipAdapters) {
            if (ipAdapter.index == row.index) {
                IfTable2Row ifRow = ifTable2RowByIndex(row.index);
                if (ifRow.valid && ifRow.interfaceType != IF_TYPE_PPP && !ifRow.isWindscribeAdapter()) {
                    const auto row_metric = static_cast<int>(row.metric);
                    if (row_metric < lowestMetric) {
                        lowestMetric = row_metric;
                        lowestMetricIfRow = ifRow;
                    }
                }
            }
        }
    }

    return lowestMetricIfRow;
}

static QList<AdapterAddress> getAdapterAddressesTable()
{
    QList<AdapterAddress> adapters;

    ULONG bufSize = sizeof(IP_ADAPTER_ADDRESSES_LH) * 32;
    QByteArray pAddresses(bufSize, Qt::Uninitialized);

    ULONG result = ::GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, (PIP_ADAPTER_ADDRESSES)pAddresses.data(), &bufSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        pAddresses.resize(bufSize);
        result = ::GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, (PIP_ADAPTER_ADDRESSES)pAddresses.data(), &bufSize);
    }

    if (result == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)pAddresses.data();
        while (pCurrAddresses) {
            QString guidString = guidToQString(pCurrAddresses->NetworkGuid);
            AdapterAddress aa(pCurrAddresses->IfIndex, guidString, QString::fromWCharArray(pCurrAddresses->FriendlyName));
            adapters.append(aa);

            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    else {
        if (result == ERROR_NO_DATA) {
            qCDebug(LOG_BASIC) << "getAdapterAddressesTable: GetAdaptersAddresses returned no addresses for the requested parameters";
        }
        else {
            wchar_t strErr[1024];
            WinUtils::Win32GetErrorString(result, strErr, _countof(strErr));
            qCDebug(LOG_BASIC) << "getAdapterAddressesTable: GetAdaptersAddresses failed (" << result << ")" << strErr;
        }
    }

    return adapters;
}


static QString networkNameFromInterfaceGUID(QString adapterGUID)
{
    QString result = "";

    INetworkListManager *pNetListManager = NULL;
    HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL,
                                  CLSCTX_ALL, IID_INetworkListManager,
                                  (LPVOID *)&pNetListManager);
    if (FAILED(hr)) {
        qCDebug(LOG_BASIC) << "Failed to create CLSID_NetworkListManager:" << hr;
        return result;
    }

    CComPtr<IEnumNetworkConnections> pEnumNetworkConnections;
    hr = pNetListManager->GetNetworkConnections(&pEnumNetworkConnections);
    if (SUCCEEDED(hr)) {
        DWORD dwReturn = 0;
        while (true) {
            CComPtr<INetworkConnection> pNetConnection;
            hr = pEnumNetworkConnections->Next(1, &pNetConnection, &dwReturn);
            if (SUCCEEDED(hr) && dwReturn > 0) {
                GUID adapterID;
                if (pNetConnection->GetAdapterId(&adapterID) == S_OK) {
                    QString adapterIDStr = guidToQString(adapterID);

                    if (adapterGUID == adapterIDStr) {
                        CComPtr<INetwork> pNetwork;
                        if (pNetConnection->GetNetwork(&pNetwork) == S_OK) {
                            BSTR bstrName = NULL;
                            if (pNetwork->GetName(&bstrName) == S_OK) {
                                result = QString::fromWCharArray(bstrName);
                                SysFreeString(bstrName);
                            }
                        }
                        break;
                    }
                }
            }
            else {
                break;
            }
        }
    }

    pEnumNetworkConnections.Release();
    pNetListManager->Release();

    return result;
}

bool NetworkUtils_win::isInterfaceSpoofed(int interfaceIndex)
{
    return interfaceSubkeyHasProperty(interfaceIndex, "WindscribeMACSpoofed");
}

bool NetworkUtils_win::pingWithMtu(const QString &url, int mtu)
{
    const QString cmd = WinUtils::getSystemDir() + QString("\\ping.exe");
    const QString params = QString(" -n 1 -l %1 -f %2").arg(mtu).arg(url);
    QString result = WinUtils::executeBlockingCmd(cmd + params, params, 1000).trimmed();
    if (result.contains("bytes=")) {
        return true;
    }
    return false;
}

QString NetworkUtils_win::getLocalIP()
{
    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
    std::vector<unsigned char> pAdapterInfo(ulAdapterInfoSize);

    if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo.resize(ulAdapterInfoSize);
    }

    if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_SUCCESS) {
        IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)&pAdapterInfo[0];
        do {
            if ((ai->Type == MIB_IF_TYPE_ETHERNET) || (ai->Type == IF_TYPE_IEEE80211)) {
                if (strcmp(ai->IpAddressList.IpAddress.String, "0.0.0.0") != 0 && strcmp(ai->GatewayList.IpAddress.String, "0.0.0.0") != 0) {
                    return ai->IpAddressList.IpAddress.String;
                }
            }
            ai = ai->Next;
        } while (ai);
    }
    return "";
}

types::NetworkInterface NetworkUtils_win::currentNetworkInterface()
{
    types::NetworkInterface curNetworkInterface = types::NetworkInterface::noNetworkInterface();

    IfTable2Row row = lowestMetricNonWindscribeIfTableRow();
    if (!row.valid) {
        return curNetworkInterface;
    }

    QVector<types::NetworkInterface> networkInterfaces = currentNetworkInterfaces(true);

    for (const auto &networkInterface : networkInterfaces) {
        if (networkInterface.interfaceIndex == static_cast<int>(row.index)) {
            curNetworkInterface = networkInterface;
            break;
        }
    }

    // Checking for a cellular modem
    // It is not detectable by lowest metric because Windows assigns it the maximum metric as a rule
    if (curNetworkInterface.isNoNetworkInterface()) {
        for (const auto &networkInterface : networkInterfaces) {
            if (networkInterface.active &&
               (networkInterface.dwType == IF_TYPE_IEEE80216_WMAN || networkInterface.dwType == IF_TYPE_WWANPP ||  networkInterface.dwType == IF_TYPE_WWANPP2)) {
                curNetworkInterface = networkInterface;
                break;
            }
        }
    }

    // qDebug() << "#########Current interface found to be: ";
    // Utils::printInterface(curNetworkInterface);
    // qDebug() << "Of: ";
    // Utils::printInterfaces(interfaces);

    return curNetworkInterface;
}

QVector<types::NetworkInterface> NetworkUtils_win::currentNetworkInterfaces(bool includeNoInterface)
{
    QVector<types::NetworkInterface> networkInterfaces;

    // Add "No Interface" selection
    if (includeNoInterface) {
        networkInterfaces << types::NetworkInterface::noNetworkInterface();
    }

    const QList<IpAdapter> ipAdapters = getIpAdapterTable();
    const QList<AdapterAddress> adapterAddresses = getAdapterAddressesTable(); // TODO: invalid parameters passed to C runtime

    const QList<IfTableRow> ifTable = getIfTable();
    const QList<IfTable2Row> ifTable2 = getIfTable2();
    const QList<IpForwardRow> ipForwardTable = getIpForwardTable();

    WlanUtils_win wlanUtils;

    for (const IpAdapter &ia: ipAdapters) { // IpAdapters holds list of live adapters
        types::NetworkInterface networkInterface;
        networkInterface.interfaceIndex = ia.index;
        networkInterface.interfaceGuid = ia.guid;
        networkInterface.physicalAddress = ia.physicalAddress;

        NETWORK_INTERFACE_TYPE nicType = NETWORK_INTERFACE_NONE;

        for (const IfTableRow &itRow: ifTable) {
            if (itRow.index == static_cast<int>(ia.index)) {
                nicType = itRow.type;
                networkInterface.mtu = itRow.mtu;
                networkInterface.interfaceType = nicType;
                networkInterface.active = itRow.connected;
                networkInterface.dwType = itRow.dwType;
                networkInterface.deviceName = itRow.interfaceName;

                if (nicType == NETWORK_INTERFACE_WIFI) {
                    DWORD ret = wlanUtils.ssidFromInterfaceGUID(itRow.guidName, networkInterface.networkOrSsid);
                    if (ret != ERROR_SUCCESS) {
                        if (ret == ERROR_ACCESS_DENIED) {
                            g_SsidAccessDenied = true;
                        }
                        networkInterface.networkOrSsid = "Wi-Fi";
                    } else {
                        g_SsidAccessDenied = false;
                    }
                } else if (nicType == NETWORK_INTERFACE_MOBILE_BROADBAND) {
                    networkInterface.networkOrSsid = itRow.interfaceName;
                }
                break;
            }
        }

        for (const IfTable2Row &it2Row: ifTable2) {
            if (it2Row.index == ia.index) {
                if (nicType == NETWORK_INTERFACE_ETH) {
                    networkInterface.networkOrSsid = networkNameFromInterfaceGUID(it2Row.interfaceGuid);
                }
                networkInterface.connectorPresent = it2Row.connectorPresent;
                networkInterface.endPointInterface = it2Row.endPointInterface;
                break;
            }
        }

        for (const IpForwardRow &ipfRow: ipForwardTable) {
            if (ipfRow.index == ia.index) {
                networkInterface.metric = ipfRow.metric;
                break;
            }
        }

        for (const AdapterAddress &aa: adapterAddresses) {
            if (aa.index == ia.index) {
                networkInterface.interfaceName = aa.friendlyName;
                networkInterface.friendlyName  = networkInterface.interfaceName;
            }
        }

        // Filter out virtual physical adapters, but keep virtual adapters that act as Ethernet
        // network bridges (e.g. Hyper-V virtual switches on the host side).
        if (networkInterface.dwType != IF_TYPE_PPP && !networkInterface.endPointInterface &&
            (networkInterface.interfaceType == NETWORK_INTERFACE_ETH || networkInterface.connectorPresent))
        {
            networkInterfaces << networkInterface;
        }
    }

    return networkInterfaces;
}

QString NetworkUtils_win::interfaceSubkeyName(int interfaceIndex)
{
    IfTableRow row = ifRowByIndex(interfaceIndex);

    const QString keyPath("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
    const QStringList ifSubkeys = WinUtils::enumerateSubkeyNames(HKEY_LOCAL_MACHINE, keyPath);

    QString foundSubkeyName = "";
    for (const QString &subkey : ifSubkeys) {
        QString subkeyPath = keyPath + "\\" + subkey;
        QString subkeyInterfaceGuid = WinUtils::regGetLocalMachineRegistryValueSz(subkeyPath, "NetCfgInstanceId");

        if (subkeyInterfaceGuid == row.guidName) {
            foundSubkeyName = subkey; // just name -- not path
            break;
        }
    }

    return foundSubkeyName;
}

types::NetworkInterface NetworkUtils_win::interfaceByIndex(int index, bool &success)
{
    types::NetworkInterface result;
    success = false;

    QVector<types::NetworkInterface> nis = currentNetworkInterfaces(true);

    for (int i = 0; i < nis.size(); i++) {
        types::NetworkInterface ni = nis[i];

        if (ni.interfaceIndex == index) {
            result = ni;
            success = true;
            break;
        }
    }

    return result;
}

std::optional<bool> NetworkUtils_win::haveInternetConnectivity()
{
    INetworkListManager* mgr = nullptr;
    HRESULT res = ::CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL,
                                     IID_INetworkListManager, reinterpret_cast<void**>(&mgr));
    if (res != S_OK) {
        qCDebug(LOG_BASIC) << "haveInternetConnectivity: could not create an INetworkListManager instance" << HRESULT_CODE(res);
        return std::nullopt;
    }

    auto comRelease = qScopeGuard([&] {
        if (mgr != nullptr) {
            mgr->Release();
        }
    });

    NLM_CONNECTIVITY connectivity;
    res = mgr->GetConnectivity(&connectivity);

    if (res != S_OK) {
        qCDebug(LOG_BASIC) << "haveInternetConnectivity: GetConnectivity failed" << HRESULT_CODE(res);
        return std::nullopt;
    }

    qCDebug(LOG_BASIC) << "haveInternetConnectivity: GetConnectivity returned" << connectivity;

    if ((connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) || (connectivity & NLM_CONNECTIVITY_IPV6_INTERNET)) {
        return true;
    }

    return false;
}

QString NetworkUtils_win::getRoutingTable()
{
    const QString cmd = WinUtils::getSystemDir() + QString("\\route.exe print");
    return WinUtils::executeBlockingCmd(cmd, "", 50).trimmed();
}

bool NetworkUtils_win::isSsidAccessAvailable()
{
    return !g_SsidAccessDenied;
}
