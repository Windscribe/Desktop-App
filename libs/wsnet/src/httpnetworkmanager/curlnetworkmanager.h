#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <condition_variable>
#include "WSNetHttpRequest.h"
#include "WSNetHttpNetworkManager.h"
#include "certmanager.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

typedef std::function<void(std::uint64_t requestId, bool bSuccess)> CurlFinishedCallback;
typedef std::function<void(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)> CurlProgressCallback;
typedef std::function<void(std::uint64_t requestId, const std::string &data)> CurlReadyDataCallback;

// Implementing queries with curl library.
class CurlNetworkManager
{
public:
    explicit CurlNetworkManager(CurlFinishedCallback finishedCallback, CurlProgressCallback progressCallback, CurlReadyDataCallback readyDataCallback);
    virtual ~CurlNetworkManager();

    bool init();

    // the calling party must take care that the requestId's are unique
    void executeRequest(std::uint64_t requestId, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips, std::uint32_t timeoutMs);
    void cancelRequest(std::uint64_t requestId);

    void setProxySettings(const std::string &address, const std::string &username, const std::string &password);

    void setWhitelistSocketsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback> > callback);

private:
    void run();

private:
    bool isCurlGlobalInitialized_ = false;
    CurlFinishedCallback finishedCallback_;
    CurlProgressCallback progressCallback_;
    CurlReadyDataCallback readyDataCallback_;

    CertManager certManager_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread thread_;
    std::atomic<bool> finish_;

    struct ProxySettings {
        std::string address;
        std::string username;
        std::string password;
    };
    ProxySettings proxySettings_;

    struct RequestInfo {
        std::uint64_t id;
        CurlNetworkManager *curlNetworkManager;
        CURL *curlEasyHandle = nullptr;
        std::vector<struct curl_slist *> curlLists;
        bool isAddedToMultiHandle = false;
        bool isNeedRemoveFromMultiHandle = false;

        // free all curl handles and data
        ~RequestInfo() {
            if (curlEasyHandle) {
                // delete our user data (pointer to quint64)
                std::uint64_t *id;
                if (curl_easy_getinfo(curlEasyHandle, CURLINFO_PRIVATE, &id) == CURLE_OK && id != nullptr)
                    delete id;
                curl_easy_cleanup(curlEasyHandle);
            }
            for (struct curl_slist *list : curlLists)
                curl_slist_free_all(list);
        }
    };

    CURLM *multiHandle_;
    std::map<std::uint64_t, RequestInfo *> activeRequests_;

    std::mutex mutexForWhiteListSockets_; // this socket protects whitelistSocketsCallback_ variable
    std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback> > whitelistSocketsCallback_;
    std::set<int> whitelistSockets_;

    static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm);
    static size_t writeDataCallback(void *ptr, size_t size, size_t count, void *ri);
    static int progressCallback(void *ri,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow);
    static int curlSocketCallback(void *clientp, curl_socket_t curlfd, curlsocktype purpose);
    static int curlCloseSocketCallback(void *clientp, curl_socket_t curlfd);


    bool setupOptions(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips, std::uint32_t timeoutMs);
    bool setupResolveHosts(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips);
    bool setupSslVerification(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request);
    bool setupProxy(RequestInfo *requestInfo);
};

} // namespace wsnet
