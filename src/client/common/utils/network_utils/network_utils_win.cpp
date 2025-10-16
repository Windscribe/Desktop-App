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

template <typename T>
static T tableRowByIndex(const QList<T> &table, int index)
{
    T found;

    for (const T &row : table) {
        if (index == static_cast<int>(row.index)) { // TODO: convert all ifIndices to int
            found = row;
            break;
        }
    }

    return found;
}

static QList<IfTableRow> getIfTable()
{
    QList<IfTableRow> table;

    // Make an initial call to GetIfTable to get the necessary size into dwSize
    DWORD dwSize = 0;
    auto result = GetIfTable(NULL, &dwSize, FALSE);
    if (result != ERROR_INSUFFICIENT_BUFFER) {
        qCCritical(LOG_BASIC) << "Initial GetIfTable unexpected failure:" << result;
        return table;
    }

    QScopedArrayPointer<unsigned char> buf(new unsigned char[dwSize]);
    PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)buf.data();

    // Make a second call to GetIfTable to get the actual data we want.
    result = GetIfTable(pIfTable, &dwSize, FALSE);
    if (result != NO_ERROR) {
        qCCritical(LOG_BASIC) << "GetIfTable failed with error:" << result;
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
        qCCritical(LOG_BASIC) << "GetIfTable2 failed:" << result;
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
                            pEntry->Type,
                            !pEntry->InterfaceAndOperStatusFlags.NotMediaConnected);
            if2Table.append(row);
        }
    }

    FreeMibTable(pIfTable2);

    return if2Table;
}

static QString interfaceSubkeyPath(int interfaceIndex)
{
    IfTableRow row = tableRowByIndex(getIfTable(), interfaceIndex);

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
            qCCritical(LOG_BASIC) << "Initial GetIpForwardTable unexpected failure:" << result;
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
        qCCritical(LOG_BASIC) << "GetIpForwardTable failed:" << result;
        return forwardTable;
    }

    for (auto i = 0; i < pIpForwardTable->dwNumEntries; i++) {
        IpForwardRow ipRow = IpForwardRow(pIpForwardTable->table[i].dwForwardIfIndex,
                                          pIpForwardTable->table[i].dwForwardMetric1);
        forwardTable.append(ipRow);
    }

    return forwardTable;
}

static QString networkNameFromInterfaceGUID(QString adapterGUID)
{
    QString result = "";

    INetworkListManager *pNetListManager = NULL;
    HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL,
                                  CLSCTX_ALL, IID_INetworkListManager,
                                  (LPVOID *)&pNetListManager);
    if (FAILED(hr)) {
        qCCritical(LOG_BASIC) << "Failed to create CLSID_NetworkListManager:" << hr;
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

static bool isRowUsableForWindscribe(const IfTable2Row &row)
{
    if (row.interfaceType == IF_TYPE_ETHERNET_CSMACD) {
        QString networkOrSsid = networkNameFromInterfaceGUID(row.interfaceGuid);
        if (networkOrSsid == "Identifying..." || networkOrSsid == "Unidentified network") {
            return false;
        }
    }

    return row.valid && row.interfaceType != IF_TYPE_PPP && !row.endPointInterface && row.connectorPresent;
}

static IfTable2Row lowestMetricNonWindscribeIfTableRow()
{
    IfTable2Row lowestMetricIfRow;

    const QList<IpForwardRow> fwdTable = getIpForwardTable();
    const QList<IfTable2Row> ifTable2 = getIfTable2();
    const QList<IpAdapter> ipAdapters = getIpAdapterTable();

    int lowestMetric = 999999;

    for (const IpForwardRow &row: fwdTable) {
        for (const IpAdapter &ipAdapter: ipAdapters) {
            if (ipAdapter.index == row.index) {
                IfTable2Row ifRow = tableRowByIndex(ifTable2, row.index);
                // We call this function to determine the current interface, so also make sure that the row is media-connected.
                if (isRowUsableForWindscribe(ifRow) && ifRow.mediaConnected && !ifRow.isWindscribeAdapter()) {
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
            qCWarning(LOG_BASIC) << "getAdapterAddressesTable: GetAdaptersAddresses returned no addresses for the requested parameters";
        }
        else {
            wchar_t strErr[1024];
            WinUtils::Win32GetErrorString(result, strErr, _countof(strErr));
            qCCritical(LOG_BASIC) << "getAdapterAddressesTable: GetAdaptersAddresses failed (" << result << ")" << strErr;
        }
    }

    return adapters;
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

QString NetworkUtils_win::currentNetworkInterfaceGuid()
{
    // This function is used primarily by network detection manager to check if the current interface has changed.
    // We do not want to use currentNetworkInterfaces() here, because that returns a cached list
    // (which is not useful to determine if the interface has changed), or it forces an update
    // (which we want to avoid until we know something actually changed).

    IfTable2Row row = lowestMetricNonWindscribeIfTableRow();
    if (!row.valid) {
        return "";
    }

    return row.interfaceGuid;
}

bool NetworkUtils_win::haveActiveInterface()
{
    // TODO: reconcile this with haveInternetConnectivity().  This function is used by NetworkDetectionManager::isOnlineImpl()
    // to determine if there is an active interface, without forcing a network list update.
    // Changing it to use haveInternetConnectivity() would reduce the amount of logic in this file, but may cause slightly different behavior.
    QList<IpAdapter> ipAdapters = getIpAdapterTable();
    QList<IfTable2Row> ifTable2 = getIfTable2();
    for (const IpAdapter &ia : ipAdapters) {
        IfTable2Row row = tableRowByIndex(ifTable2, ia.index);
        if (isRowUsableForWindscribe(row) && row.mediaConnected) {
            return true;
        }
    }
    return false;
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

    // qDebug() << "#########Current interface found to be: ";
    // Utils::printInterface(curNetworkInterface);
    // qDebug() << "Of: ";
    // Utils::printInterfaces(interfaces);

    return curNetworkInterface;
}

static QVector<types::NetworkInterface> addNoInterface(bool includeNoInterface, const QVector<types::NetworkInterface> &networkInterfaces)
{
    if (includeNoInterface) {
        QVector<types::NetworkInterface> ret;
        ret << types::NetworkInterface::noNetworkInterface();
        ret << networkInterfaces;
        return ret;
    } else {
        return networkInterfaces;
    }
}

QVector<types::NetworkInterface> NetworkUtils_win::currentNetworkInterfaces(bool includeNoInterface, bool forceUpdate)
{
    static QVector<types::NetworkInterface> networkInterfaces;
    if (!forceUpdate && !networkInterfaces.isEmpty()) {
        return addNoInterface(includeNoInterface, networkInterfaces);
    }
    qCInfo(LOG_BASIC) << "Clearing cached network interfaces";
    networkInterfaces.clear();

    WlanUtils_win wlanUtils;

    for (const IpAdapter &ia: getIpAdapterTable()) { // IpAdapters holds list of live adapters
        types::NetworkInterface networkInterface;
        networkInterface.interfaceIndex = ia.index;
        networkInterface.interfaceGuid = ia.guid;
        networkInterface.physicalAddress = ia.physicalAddress;

        IfTableRow itRow = tableRowByIndex(getIfTable(), ia.index);
        if (!itRow.valid) {
            // Should never happen
            assert(false);
            continue;
        }

        networkInterface.mtu = itRow.mtu;
        networkInterface.interfaceType = itRow.type;
        networkInterface.active = itRow.connected;

        if (networkInterface.interfaceType == NETWORK_INTERFACE_WIFI) {
            DWORD ret = wlanUtils.ssidFromInterfaceGUID(itRow.guidName, networkInterface.networkOrSsid);
            if (ret != ERROR_SUCCESS) {
                if (ret == ERROR_ACCESS_DENIED) {
                    g_SsidAccessDenied = true;
                }
                networkInterface.networkOrSsid = "Wi-Fi";
            } else {
                g_SsidAccessDenied = false;
            }
        } else if (networkInterface.interfaceType == NETWORK_INTERFACE_MOBILE_BROADBAND) {
            networkInterface.networkOrSsid = itRow.interfaceName;
        }

        AdapterAddress aa = tableRowByIndex(getAdapterAddressesTable(), ia.index);
        if (aa.valid) {
            networkInterface.interfaceName = aa.friendlyName;
            networkInterface.friendlyName  = networkInterface.interfaceName;
        }

        IfTable2Row it2Row = tableRowByIndex(getIfTable2(), ia.index);
        if (!it2Row.valid) {
            // Should never happen
            assert(false);
            continue;
        }
        if (!isRowUsableForWindscribe(it2Row)) {
            continue;
        }

        if (networkInterface.interfaceType == NETWORK_INTERFACE_ETH) {
            networkInterface.networkOrSsid = networkNameFromInterfaceGUID(it2Row.interfaceGuid);
            if (networkInterface.networkOrSsid.isEmpty()) {
                // Above function could not detect interface, probably virtual.  Set it to the friendly name instead.
                networkInterface.networkOrSsid = networkInterface.friendlyName;
            }
        }

        IpForwardRow ipfRow = tableRowByIndex(getIpForwardTable(), ia.index);
        if (!ipfRow.valid) {
            networkInterface.metric = -1;
        } else {
            networkInterface.metric = ipfRow.metric;
        }

        networkInterfaces << networkInterface;
    }

    return addNoInterface(includeNoInterface, networkInterfaces);
}

QString NetworkUtils_win::interfaceSubkeyName(int interfaceIndex)
{
    IfTableRow row = tableRowByIndex(getIfTable(), interfaceIndex);

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
        qCCritical(LOG_BASIC) << "haveInternetConnectivity: could not create an INetworkListManager instance" << HRESULT_CODE(res);
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
        qCCritical(LOG_BASIC) << "haveInternetConnectivity: GetConnectivity failed" << HRESULT_CODE(res);
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
