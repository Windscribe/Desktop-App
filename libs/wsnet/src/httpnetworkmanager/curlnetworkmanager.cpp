#include "curlnetworkmanager.h"
#include <regex>
#include "utils/wsnet_logger.h"
#include "utils/utils.h"
#include "utils/crypto_utils.h"
#include "settings.h"

#include <openssl/opensslv.h>
#include <openssl/ssl.h>

#ifdef _WIN32
#else
    #include <unistd.h>
#endif
namespace wsnet {

CurlNetworkManager::CurlNetworkManager(CurlFinishedCallback finishedCallback, CurlProgressCallback progressCallback, CurlReadyDataCallback readyDataCallback) :
    finishedCallback_(finishedCallback), progressCallback_(progressCallback), readyDataCallback_(readyDataCallback),
    finish_(false), multiHandle_(nullptr)
{
}

CurlNetworkManager::~CurlNetworkManager()
{
    finish_ = true;
    condition_.notify_all();
    thread_.join();

    if (isCurlGlobalInitialized_)
        curl_global_cleanup();
}

bool CurlNetworkManager::init()
{
    if (!isCurlGlobalInitialized_) {
        g_logger->info("openssl version: {}", OPENSSL_FULL_VERSION_STR);
        g_logger->info("curl version: {}", LIBCURL_VERSION);
        if (curl_global_sslset(CURLSSLBACKEND_OPENSSL, nullptr, nullptr) != CURLSSLSET_OK) {
            g_logger->critical("curl_global_sslset failed");
            return false;
        }

        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            g_logger->critical("curl_global_init failed");
            return false;
        }
        isCurlGlobalInitialized_ = true;

        multiHandle_ = curl_multi_init();
        thread_ = std::thread(std::bind(&CurlNetworkManager::run, this));
    }
    return true;
}

void CurlNetworkManager::executeRequest(std::uint64_t requestId, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips, std::uint32_t timeoutMs)
{
    RequestInfo *requestInfo = new RequestInfo();
    requestInfo->id = requestId;
    requestInfo->curlNetworkManager = this;
    requestInfo->curlEasyHandle = curl_easy_init();
    requestInfo->isDebugLogCurlError = request->isDebugLogCurlError();

    // Prepare data for debug log privacy
    if (requestInfo->isDebugLogCurlError) {
        requestInfo->domain = utils::topDomain(request->hostname());
        requestInfo->domainMd5 = crypto_utils::md5(requestInfo->domain);
        requestInfo->ips = ips;
        for (const auto &it : requestInfo->ips) {
            requestInfo->ipsMd5.push_back(crypto_utils::md5(it));
        }
    }

    if (requestInfo->curlEasyHandle)  {
        std::lock_guard locker(mutex_);
        if (setupOptions(requestInfo, request, ips, timeoutMs)) {
            activeRequests_[requestId] = requestInfo;
            condition_.notify_all();
            curl_multi_wakeup(multiHandle_);
            return;
        }
    }
    // if we here then something failed
    delete requestInfo;
    assert(false);
}

void CurlNetworkManager::cancelRequest(std::uint64_t requestId)
{
    std::lock_guard locker(mutex_);
    auto it = activeRequests_.find(requestId);
    if (it != activeRequests_.end())
        it->second->isNeedRemoveFromMultiHandle = true;
}

void CurlNetworkManager::setProxySettings(const std::string &address, const std::string &username, const std::string &password)
{
    std::lock_guard locker(mutex_);
    proxySettings_.address = address;
    proxySettings_.username = username;
    proxySettings_.password = password;
}

void CurlNetworkManager::setWhitelistSocketsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback> > callback)
{
    std::lock_guard locker(mutexForWhiteListSockets_);
    whitelistSocketsCallback_ = callback;
}

void CurlNetworkManager::run()
{
    while (!finish_) {

        {
            std::unique_lock<std::mutex> locker(mutex_);
            while (activeRequests_.empty() && !finish_)
                condition_.wait(locker);
        }

        if (finish_)
            break;

        {
            std::lock_guard locker(mutex_);
            // add curl handles for requests which have not been added
            for (auto it = activeRequests_.begin(); it != activeRequests_.end(); ++it) {
                if (!it->second->isAddedToMultiHandle) {
                    curl_multi_add_handle(multiHandle_, it->second->curlEasyHandle);
                    it->second->isAddedToMultiHandle = true;
                }
            }

            // check for need to be deleted requests (for cases when the function abort() was called)
            for (auto it = activeRequests_.begin(); it != activeRequests_.end(); /*++it*/) {  // should not increment it in the for loop
                if (it->second->isNeedRemoveFromMultiHandle) {
                    RequestInfo *ri = it->second;
                    if (it->second->isAddedToMultiHandle)
                        curl_multi_remove_handle(multiHandle_, ri->curlEasyHandle);
                    it = activeRequests_.erase(it);
                    delete ri;
                } else {
                    ++it;
                }
            }
        }

        int still_running;
        curl_multi_perform(multiHandle_, &still_running);
        // check finished requests
        struct CURLMsg *curlMsg = nullptr;
        do {
            int msgq = 0;
            curlMsg = curl_multi_info_read(multiHandle_, &msgq);
            if (curlMsg && (curlMsg->msg == CURLMSG_DONE)) {
                CURL *curlEasyHandle = curlMsg->easy_handle;
                std::uint64_t *pointerId;
                curl_easy_getinfo(curlEasyHandle, CURLINFO_PRIVATE, &pointerId);
                assert(pointerId != nullptr);

                curl_off_t totalTime;
                curl_easy_getinfo(curlEasyHandle, CURLINFO_TOTAL_TIME_T, &totalTime);

                curl_multi_remove_handle(multiHandle_, curlEasyHandle);

                CURLcode result = curlMsg->data.result;

                std::uint64_t id;
                //remove request from activeRequests
                {
                    std::lock_guard locker(mutex_);
                    id = *pointerId;
                    auto it = activeRequests_.find(id);
                    assert(it != activeRequests_.end());
                    assert(it->second->curlEasyHandle == curlEasyHandle);

                    if (result != CURLE_OK) {
                        g_logger->debug("Curl request error: {}", curl_easy_strerror(result));

                        // Log all curl output for a failed request
                        if (it->second->isDebugLogCurlError) {
                            for (const auto &log: it->second->debugLogs) {
                                g_logger->info("{}", log);
                            }
                        }
                    } else {
                        // Log curl output for a successful request, only strings containing "Trying" and "Connected" substrings to reduce log bloat
                        if (it->second->isDebugLogCurlError) {
                            for (const auto &log: it->second->debugLogs) {
                                if (log.find("Trying") != std::string::npos || log.find("Connected") != std::string::npos) {
                                    g_logger->info("{}", log);
                                }
                            }
                        }
                    }

                    delete it->second;
                    activeRequests_.erase(id);
                }
                finishedCallback_(id, result == CURLE_OK, curl_easy_strerror(result));
            }
        } while(curlMsg);


        // wait for activity or timeout
        int numfds;
        curl_multi_poll(multiHandle_, NULL, 0, 1000, &numfds);
    }

    for (auto it = activeRequests_.begin(); it != activeRequests_.end(); ++it)
        delete it->second;
    activeRequests_.clear();
}

CURLcode CurlNetworkManager::sslctx_function(CURL *curl, void *sslctx, void *parm)
{
    X509_STORE *store = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
    CertManager *certManager = static_cast<CertManager *>(parm);
    for (int i = 0; i < certManager->count(); ++i)
        X509_STORE_add_cert(store, certManager->getCert(i));

    return CURLE_OK;
}

size_t CurlNetworkManager::writeDataCallback(void *ptr, size_t size, size_t count, void *ri)
{
    RequestInfo *requestInfo = static_cast<RequestInfo *>(ri);
    std::string data((char *)ptr, (char *)ptr + size * count);
    requestInfo->curlNetworkManager->readyDataCallback_(requestInfo->id, data);
    return size*count;
}

int CurlNetworkManager::progressCallback(void *ri, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    RequestInfo *requestInfo = static_cast<RequestInfo *>(ri);
    if (dltotal > 0) {
        requestInfo->curlNetworkManager->progressCallback_(requestInfo->id, dlnow, dltotal);
    }
    return 0;
}

int CurlNetworkManager::curlSocketCallback(void *clientp, curl_socket_t curlfd, curlsocktype purpose)
{
    CurlNetworkManager *this_ = (CurlNetworkManager *)clientp;

    std::lock_guard locker(this_->mutexForWhiteListSockets_);
    // whitelist the new socket descriptor
    if (this_->whitelistSockets_.find(curlfd) == this_->whitelistSockets_.end()) {
        this_->whitelistSockets_.insert(curlfd);
        if (this_->whitelistSocketsCallback_) {
            this_->whitelistSocketsCallback_->call(this_->whitelistSockets_);
        }
    }
    return CURL_SOCKOPT_OK;
}

int CurlNetworkManager::curlCloseSocketCallback(void *clientp, curl_socket_t curlfd)
{
    CurlNetworkManager *this_ = (CurlNetworkManager *)clientp;
#ifdef _WIN32
    closesocket(curlfd);
#else
    close(curlfd);
#endif
    std::lock_guard locker(this_->mutexForWhiteListSockets_);
    // whitelist the deleted socket descriptor
    if (this_->whitelistSockets_.find(curlfd) != this_->whitelistSockets_.end()) {
        this_->whitelistSockets_.erase(curlfd);
        if (this_->whitelistSocketsCallback_) {
            this_->whitelistSocketsCallback_->call(this_->whitelistSockets_);
        }
    }

    return CURL_SOCKOPT_OK;
}

int CurlNetworkManager::curlTrace(CURL *handle, curl_infotype type, char *data, size_t size, void *clientp)
{
    RequestInfo *requestInfo = static_cast<RequestInfo *>(clientp);

    if (type == CURLINFO_TEXT) {
        // replace all domains in the string with their md5 for privacy.
        std::string src = std::string(data, size);
        std::regex reg(requestInfo->domain);
        std::string res = regex_replace(src, reg, requestInfo->domainMd5);

        // replace all IPv4 addresses in the string with their md5 for privacy.
        for (size_t i = 0; i < requestInfo->ips.size(); ++i) {
            std::regex reg(requestInfo->ips[i]);
            res = regex_replace(res, reg, requestInfo->ipsMd5[i]);
        }

        requestInfo->debugLogs.push_back(res);
    }
    return 0;
}

bool CurlNetworkManager::setupOptions(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips, std::uint32_t timeoutMs)
{
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_WRITEFUNCTION, writeDataCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_WRITEDATA, requestInfo) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_URL, request->url().c_str()) != CURLE_OK) return false;

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SOCKOPTFUNCTION, curlSocketCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SOCKOPTDATA, this) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CLOSESOCKETFUNCTION, curlCloseSocketCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CLOSESOCKETDATA, this) != CURLE_OK) return false;

    g_logger->debug("New curl request : {}", request->url().c_str());

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_FRESH_CONNECT, 1L) != CURLE_OK) return false;
    // make connection get closed at once after use
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_FORBID_REUSE, 1L) != CURLE_OK) return false;

    // timeout for the connect phase, this timeout only limits the connection phase, it has no impact once libcurl has connected
    // I noticed that this time curl distributes equally between all IP addresses of the domain,
    // so we have to multiply this by the number of IP addresses
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CONNECTTIMEOUT_MS , timeoutMs * ips.size()) != CURLE_OK) return false;

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_XFERINFOFUNCTION, progressCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_XFERINFODATA, requestInfo) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_NOPROGRESS, 0) != CURLE_OK) return false;

    if (requestInfo->isDebugLogCurlError) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_DEBUGFUNCTION, curlTrace) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_DEBUGDATA, requestInfo) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_VERBOSE, 1L) != CURLE_OK) return false;
    }

    struct curl_slist *list = NULL;
    list = curl_slist_append(list, request->contentTypeHeader().c_str());
    if (list == NULL) return false;

    // set the user-agent request header
    std::string userAgentHeader = "User-Agent: Windscribe/" + Settings::instance().appVersion() + " (" + Settings::instance().platformName() + ")";
    list = curl_slist_append(list, userAgentHeader.c_str());

    if (!request->sniDomain().empty()) {
        std::string temp = "Host: " + request->hostname();
        list = curl_slist_append(list, temp.c_str());
        if (list == NULL) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_URL, request->sniUrl().c_str()) != CURLE_OK) return false;
    }

    requestInfo->curlLists.push_back(list);
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_HTTPHEADER, list) != CURLE_OK) return false;

    if (!setupResolveHosts(requestInfo, request, ips)) return false;
    if (!setupSslVerification(requestInfo, request)) return false;
    if (!setupProxy(requestInfo)) return false;

    if (!request->echConfig().empty()) {
        std::string tempStr = "ECL:" + request->echConfig();
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_ECH, tempStr.c_str()) != CURLE_OK)
            return false;
    }

    if (request->isExtraTLSPadding()) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_TLSEXT_PADDING | CURLSSLOPT_TLSEXT_PADDING_SUPER) != CURLE_OK)
            return false;
    }

    curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PRIVATE, new std::uint64_t(requestInfo->id));    // our user data, must be deleted in the RequestInfo destructor

    // set post data
    std::string postData = request->postData();
    if (!postData.empty()) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_POSTFIELDSIZE, postData.size()) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_COPYPOSTFIELDS, postData.c_str()) != CURLE_OK) return false;
    }

    // set additional put and delete request options
    if (request->method() == HttpMethod::kPut) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CUSTOMREQUEST, "PUT") != CURLE_OK) return false;
    } else if (request->method() == HttpMethod::kDelete) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK) return false;
    }
    return true;
}

bool CurlNetworkManager::setupResolveHosts(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request, const std::vector<std::string> &ips)
{
    if (!ips.empty()) {
        std::uint16_t port = request->port();
        if (port == 0) //  use 443 by default
            port = 443;

        std::string strResolve = request->hostname() + ":" + std::to_string(port) + ":" + utils::join(ips, ",");
        struct curl_slist *hosts = curl_slist_append(NULL, strResolve.c_str());
        if (hosts == NULL) return false;
        requestInfo->curlLists.push_back(hosts);
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_RESOLVE, hosts) != CURLE_OK) return false;
    } else {
        assert(false);
    }
    return true;
}

bool CurlNetworkManager::setupSslVerification(RequestInfo *requestInfo, const std::shared_ptr<WSNetHttpRequest> &request)
{
    if (request->isIgnoreSslErrors())  {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 0) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYHOST, 0) != CURLE_OK) return false;
    } else  {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CAINFO, 0) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CAPATH, 0) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 1) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_CTX_FUNCTION, *sslctx_function) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_CTX_DATA, &certManager_) != CURLE_OK) return false;
    }

    return true;
}

bool CurlNetworkManager::setupProxy(RequestInfo *requestInfo)
{
    std::string proxyString;

    if (proxySettings_.address.empty())
        return true;    //nothing todo

    proxyString = proxySettings_.address;

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXY, proxyString.c_str()) != CURLE_OK)
        return false;

    if (!proxySettings_.username.empty())  {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXYUSERNAME, proxySettings_.username.c_str()) != CURLE_OK)
            return false;
    }
    if (!proxySettings_.password.empty()) {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXYPASSWORD, proxySettings_.password.c_str()) != CURLE_OK)
            return false;
    }
    return true;
}

} // namespace wsnet

