#include "all_headers.h"

#include <spdlog/spdlog.h>

#include "network_category.h"
#include "../../../client/common/utils/wsscopeguard.h"

NetworkCategoryManager::NetworkCategoryManager()
    : work_(boost::asio::make_work_guard(io_context_))
    , cancel_(false)
{
    io_thread_ = std::thread([this]() { io_context_.run(); });
}

NetworkCategoryManager::~NetworkCategoryManager()
{
    cancel_ = true;
    work_.reset();
    io_context_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

NetworkCategoryManager& NetworkCategoryManager::instance()
{
    static NetworkCategoryManager instance;
    return instance;
}

bool NetworkCategoryManager::setCategory(const std::wstring& networkName, NetworkCategory category)
{
    // Cancel any current task that's still waiting
    cancel_ = true;

    boost::asio::post(io_context_, [this, networkName, category]() {
        // Reset cancel flag when starting new task
        cancel_ = false;

        for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
            // Check if task was cancelled
            if (cancel_) {
                return;
            }

            if (setCategoryInternal(networkName, category)) {
                return;
            }
            Sleep(kRetryDelayMs);
        }
        spdlog::error("NetworkCategoryManager::setCategory - could not set category after {} attempts", kMaxAttempts);
    });

    return true;
}

bool NetworkCategoryManager::setCategoryInternal(const std::wstring& networkName, NetworkCategory category)
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        spdlog::error("NetworkCategoryManager::setCategoryInternal - CoInitializeEx failed: {}", hr);
        return false;
    }

    // Create Network List Manager instance
    INetworkListManager *pNetworkListManager = NULL;
    hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL,
                            IID_INetworkListManager, (LPVOID *)&pNetworkListManager);
    if (FAILED(hr)) {
        spdlog::error("NetworkCategoryManager::setCategoryInternal - CoCreateInstance failed: {}", hr);
        CoUninitialize();
        return false;
    }

    // Get networks enumeration
    IEnumNetworks *pEnumNetworks = NULL;
    hr = pNetworkListManager->GetNetworks(NLM_ENUM_NETWORK_ALL, &pEnumNetworks);
    if (FAILED(hr)) {
        spdlog::error("NetworkCategoryManager::setCategoryInternal - GetNetworks failed: {}", hr);
        pNetworkListManager->Release();
        CoUninitialize();
        return false;
    }

    // Setup cleanup of COM resources
    auto cleanup = wsl::wsScopeGuard([pNetworkListManager, pEnumNetworks]() {
        if (pEnumNetworks) {
            pEnumNetworks->Release();
        }
        if (pNetworkListManager) {
            pNetworkListManager->Release();
        }
        CoUninitialize();
    });

    INetwork *pNetwork = NULL;
    ULONG fetched = 0;

    // Enumerate networks
    spdlog::debug(L"NetworkCategoryManager::setCategoryInternal - Setting network category for network: {}", networkName);
    bool found = false;
    while (pEnumNetworks->Next(1, &pNetwork, &fetched) == S_OK && fetched > 0) {
        if (!pNetwork) {
            continue;
        }

        auto networkGuard = wsl::wsScopeGuard([pNetwork]() {
            pNetwork->Release();
        });

        // Get and log network name and GUIDs
        BSTR name = nullptr;
        hr = pNetwork->GetName(&name);
        if (FAILED(hr)) {
            spdlog::info(L"NetworkCategoryManager::setCategoryInternal - Could not get network name");
            continue;
        }

        std::wstring wName(name);
        if (wName == networkName) {
            spdlog::info("NetworkCategoryManager::setCategoryInternal - Setting category for matching network");

            // Set the category on the network
            hr = pNetwork->SetCategory(static_cast<NLM_NETWORK_CATEGORY>(category));
            if (FAILED(hr)) {
                spdlog::error("NetworkCategoryManager::setCategoryInternal - SetCategory failed: {}", hr);
                return false;
            }
            return true;
        }
    }
    return false;
}
