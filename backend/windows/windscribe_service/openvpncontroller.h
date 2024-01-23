#pragma once

#include "wintun.h"

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

private:
    explicit OpenVPNController();

    bool useDCODriver_ = false;
    HMODULE wintunDLL_ = nullptr;
    WINTUN_ADAPTER_HANDLE adapterHandle_ = nullptr;

    bool createDCOAdapter();
    void removeDCOAdapter();

    bool createWintunAdapter();
    void removeWintunAdapter();
};
