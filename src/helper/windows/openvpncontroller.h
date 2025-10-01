#pragma once

#include "wintun.h"
#include "executecmd.h"

class OpenVPNController final
{
public:
    static OpenVPNController &instance()
    {
        static OpenVPNController wc;
        return wc;
    }

    void release();

    bool createAdapter(bool useDCODriver);
    void removeAdapter();

    ExecuteCmdResult runOpenvpn(std::wstring &config, unsigned int port, const std::wstring &httpProxy, unsigned int httpPort,
                                const std::wstring &socksProxy, unsigned int socksPort);

private:
    explicit OpenVPNController();

    bool useDCODriver_ = false;
    HMODULE wintunDLL_ = nullptr;
    WINTUN_ADAPTER_HANDLE adapterHandle_ = nullptr;

    bool createDCOAdapter();
    void removeDCOAdapter();

    bool createWintunAdapter();
    void removeWintunAdapter();

    bool writeOVPNFile(std::wstring &config, unsigned int port, const std::wstring &httpProxy, unsigned int httpPort,
                       const std::wstring &socksProxy, unsigned int socksPort, std::wstring &filename);
};
