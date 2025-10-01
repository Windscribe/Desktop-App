#pragma once

#include <NetCon.h>

#include <string>
#include <vector>

#include "miniatl.h"

typedef void (WINAPI *PFNNcFreeNetconProperties)(NETCON_PROPERTIES* pProps);

class IcsManager
{
public:
    static IcsManager &instance()
    {
        static IcsManager im;
        return im;
    }

    void release();

    bool isSupported();

    bool change(const std::wstring &adapterName);
    bool stop();

private:
    HMODULE hDll_ = nullptr;
    PFNNcFreeNetconProperties ncFreeNetconProperties_ = nullptr;

    CComPtr<INetSharingManager> pNSM_;

    // current settled private and public configurations or null if not settled
    CComPtr<INetSharingConfiguration> pNetConfigurationPrivate_;
    CComPtr<INetSharingConfiguration> pNetConfigurationPublic_;

    IcsManager();
    std::vector< CComPtr<INetConnection> > getAllConnections(CComPtr<INetSharingManager> &pNSM);
    void disableIcsOnAll(CComPtr<INetSharingManager> &pNSM, std::vector<CComPtr<INetConnection> > &pNetConnections);
};
