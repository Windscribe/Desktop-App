#pragma once
#include <string>
#include <vector>
#include <NetCon.h>
#include <memory>

#include "miniatl.h"

typedef void (WINAPI *PFNNcFreeNetconProperties)(NETCON_PROPERTIES* pProps);

class IcsManager
{
public:
	IcsManager();
    ~IcsManager();

	bool isSupported();

    bool change(const std::wstring &adapterName);
    bool stop();

private:
    HMODULE hDll_;
    PFNNcFreeNetconProperties ncFreeNetconProperties_;

    CComPtr<INetSharingManager> pNSM_;

    // current settled private and public configurations or null if not settled
    CComPtr<INetSharingConfiguration> pNetConfigurationPrivate_;
    CComPtr<INetSharingConfiguration> pNetConfigurationPublic_;

    std::vector< CComPtr<INetConnection> > getAllConnections(CComPtr<INetSharingManager> &pNSM);
    void disableIcsOnAll(CComPtr<INetSharingManager> &pNSM, std::vector<CComPtr<INetConnection> > &pNetConnections);
};

