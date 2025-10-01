#include "IcsManager.h"

#include <wtsapi32.h>
#include <spdlog/spdlog.h>
#include "../utils.h"

IcsManager::IcsManager()
{
    std::wstring dllPath = Utils::getSystemDir() + L"\\netshell.dll";
    hDll_ = LoadLibrary(dllPath.c_str());
    if (!hDll_) {
        spdlog::error("Failed to load netshell.dll ({})", ::GetLastError());
        return;
    }

    ncFreeNetconProperties_ = (PFNNcFreeNetconProperties)GetProcAddress(hDll_, "NcFreeNetconProperties");

    if (!ncFreeNetconProperties_) {
        spdlog::error("Failed to load the function NcFreeNetconProperties() from netshell.dll");
    }

    HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL, __uuidof(INetSharingManager), (void**)&pNSM_);
    if (hr != S_OK) {
        pNSM_ = nullptr;
        spdlog::error("Failed to create INetSharingManager instance");
    }
}

void IcsManager::release()
{
    pNetConfigurationPrivate_.Release();
    pNetConfigurationPublic_.Release();
    pNSM_.Release();

    if (hDll_) {
        FreeLibrary(hDll_);
        hDll_ = nullptr;
    }
}

bool IcsManager::isSupported()
{
    if (!pNSM_) {
        spdlog::warn("Get INetSharingManager failed");
        return false;
    }

    VARIANT_BOOL bInstalled = VARIANT_FALSE;
    if (pNSM_->get_SharingInstalled(&bInstalled) != S_OK)
        return false;

    return (bInstalled == VARIANT_TRUE);
}

bool IcsManager::change(const std::wstring &adapterName)
{
    if (!pNSM_) {
        spdlog::warn("Get INetSharingManager failed");
        return false;
    }

    std::vector< CComPtr<INetConnection> > pNetConnections = getAllConnections(pNSM_);

    HRESULT hr;
    CComPtr<INetConnection> pNetConnectionPublic;
    CComPtr<INetConnection> pNetConnectionPrivate;
    VARIANT varItem;

    for (auto &it : pNetConnections) {
        NETCON_PROPERTIES *ppProps = NULL;
        hr = it->GetProperties(&ppProps);
        if (hr == S_OK) {

            if (wcsstr(ppProps->pszwDeviceName, L"Microsoft Wi-Fi Direct Virtual Adapter") != nullptr) {
                pNetConnectionPrivate = it;
            } else if (wcsstr(ppProps->pszwDeviceName, adapterName.c_str()) != nullptr) {
                pNetConnectionPublic = it;
            } else if (wcsstr(ppProps->pszwName, adapterName.c_str()) != nullptr) {
                pNetConnectionPublic = it;
            }

            if (ncFreeNetconProperties_)
                ncFreeNetconProperties_(ppProps);
        }
    }

    if (pNetConnectionPublic == nullptr) {
        spdlog::warn("Not found public network connection by name");
        return false;
    }
    if (pNetConnectionPrivate == nullptr) {
        spdlog::warn("Not found private network connection by name");
        return false;
    }

    // get configuration interfaces
    CComPtr<INetSharingConfiguration> pNetConfigurationPublic;
    CComPtr<INetSharingConfiguration> pNetConfigurationPrivate;

    hr = pNSM_->get_INetSharingConfigurationForINetConnection(pNetConnectionPublic, &pNetConfigurationPublic);
    if (hr != S_OK) {
        spdlog::warn("get_INetSharingConfigurationForINetConnection failed, {}", hr);
        return false;
    }
    hr = pNSM_->get_INetSharingConfigurationForINetConnection(pNetConnectionPrivate, &pNetConfigurationPrivate);
    if (hr != S_OK) {
        spdlog::warn("get_INetSharingConfigurationForINetConnection failed, {}", hr);
        return false;
    }

    // Check if the ICS on public/private interfaces are already enabled.
    // if so, no need to change.
    VARIANT_BOOL bPublicEnabled = VARIANT_FALSE;
    SHARINGCONNECTIONTYPE typePublic = ICSSHARINGTYPE_PUBLIC;
    hr = pNetConfigurationPublic->get_SharingEnabled(&bPublicEnabled);
    if (hr != S_OK) {
        spdlog::warn("get_SharingEnabled failed for public guid, {}", hr);
        return false;
    }
    hr = pNetConfigurationPublic->get_SharingConnectionType(&typePublic);
    if (hr != S_OK) {
        spdlog::warn("get_SharingConnectionType failed for public guid, {}", hr);
        return false;
    }

    VARIANT_BOOL bPrivateEnabled = VARIANT_FALSE;
    SHARINGCONNECTIONTYPE typePrivate = ICSSHARINGTYPE_PUBLIC;
    hr = pNetConfigurationPrivate->get_SharingEnabled(&bPrivateEnabled);
    if (hr != S_OK) {
        spdlog::warn("get_SharingEnabled failed for private guid, {}", hr);
        return false;
    }
    hr = pNetConfigurationPrivate->get_SharingConnectionType(&typePrivate);
    if (hr != S_OK) {
        spdlog::warn("get_SharingConnectionType failed for private guid, {}", hr);
        return false;
    }

    if (bPublicEnabled == VARIANT_TRUE && typePublic == ICSSHARINGTYPE_PUBLIC &&
        bPrivateEnabled == VARIANT_TRUE && typePrivate == ICSSHARINGTYPE_PRIVATE) {
        // nothing need to change;
        return true;
    }

    // Disable existing ICS first, otherwise it won't work
    if (pNetConfigurationPublic_) {
        pNetConfigurationPublic_->DisableSharing();
        pNetConfigurationPublic_ = nullptr;
    }

    if (pNetConfigurationPrivate_) {
        pNetConfigurationPrivate_->DisableSharing();
        pNetConfigurationPrivate_ = nullptr;
    }

    disableIcsOnAll(pNSM_, pNetConnections);

    hr = pNetConfigurationPublic->EnableSharing(ICSSHARINGTYPE_PUBLIC);
    if (hr != S_OK) {
        spdlog::warn("EnableSharing failed for public guid, {}", hr);
        return false;
    } else {
        pNetConfigurationPublic_ = pNetConfigurationPublic;
    }

    hr = pNetConfigurationPrivate->EnableSharing(ICSSHARINGTYPE_PRIVATE);
    if (hr != S_OK) {
        spdlog::warn("EnableSharing failed for private guid, {}", hr);
        return false;
    } else {
        pNetConfigurationPrivate_ = pNetConfigurationPrivate;
    }

    return true;
}

bool IcsManager::stop()
{
    if (pNetConfigurationPublic_) {
        pNetConfigurationPublic_->DisableSharing();
        pNetConfigurationPublic_ = nullptr;
    }

    if (pNetConfigurationPrivate_) {
        pNetConfigurationPrivate_->DisableSharing();
        pNetConfigurationPrivate_ = nullptr;
    }

    if (!pNSM_) {
        spdlog::warn("Get INetSharingManager failed");
        return false;
    }

    std::vector< CComPtr<INetConnection> > pNetConnections = getAllConnections(pNSM_);
    disableIcsOnAll(pNSM_, pNetConnections);
    return true;
}

std::vector<CComPtr<INetConnection> > IcsManager::getAllConnections(CComPtr<INetSharingManager> &pNSM)
{
    std::vector<CComPtr<INetConnection> > ret;

    CComPtr<INetSharingEveryConnectionCollection> pConnectionsList;
    HRESULT hr = pNSM->get_EnumEveryConnection(&pConnectionsList);
    if (hr != S_OK) {
        spdlog::warn("get_EnumEveryConnection failed, {}", hr);
        return ret;
    }

    CComPtr<IUnknown> pUnkEnum;
    hr = pConnectionsList->get__NewEnum(&pUnkEnum);
    if (hr != S_OK) {
        spdlog::warn("get__NewEnum failed, {}", hr);
        return ret;
    }

    CComPtr<IEnumNetSharingEveryConnection> pNSEConn;
    hr = pUnkEnum->QueryInterface(__uuidof(IEnumNetSharingEveryConnection), (void**)&pNSEConn);
    if (hr != S_OK) {
        spdlog::warn("QueryInterface IEnumNetSharingEveryConnection failed, {}", hr);
        return ret;
    }

    CComPtr<INetConnection> pNetConnection;
    VARIANT varItem;

    while (1) {
        pNetConnection = NULL;
        VariantClear(&varItem);

        hr = pNSEConn->Next(1, &varItem, NULL);
        if (S_FALSE == hr) {
            break;
        }

        if ((V_VT(&varItem) == VT_UNKNOWN) && V_UNKNOWN(&varItem)) {
            hr = V_UNKNOWN(&varItem)->QueryInterface(__uuidof(INetConnection), (void**)&pNetConnection);
            if (hr == S_OK) {
                ret.push_back(pNetConnection);
            }
        }
    }
    return ret;
}

void IcsManager::disableIcsOnAll(CComPtr<INetSharingManager> &pNSM, std::vector<CComPtr<INetConnection> > &pNetConnections)
{
    for (size_t i = 0; i < pNetConnections.size(); ++i) {
        CComPtr<INetSharingConfiguration> pNetConfiguration;
        HRESULT hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnections[i], &pNetConfiguration);
        if (hr == S_OK) {
            VARIANT_BOOL bEnabled = VARIANT_FALSE;
            hr = pNetConfiguration->get_SharingEnabled(&bEnabled);
            if (hr == S_OK && bEnabled == VARIANT_TRUE) {
                pNetConfiguration->DisableSharing();
            }
        }
    }
}
