#pragma once

#include <string>

#include "wintun.h"

class WintunController final
{
public:
    static WintunController &instance()
    {
        static WintunController wc;
        return wc;
    }

    void release();

    bool createAdapter();
    void removeAdapter();

    static std::wstring adapterName();

private:
    explicit WintunController();

    HMODULE wintunDLL_ = nullptr;
    WINTUN_ADAPTER_HANDLE adapterHandle_ = nullptr;
};
