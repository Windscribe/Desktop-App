#include "curlnetworkmanager_impl.h"
#include <curl/curl.h>
#include <QMap>
#include <QDebug>
#include <QCoreApplication>
#include <openssl/ssl.h>
#include "utils/ws_assert.h"
//#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QStandardPaths>

CurlNetworkManagerImpl *g_this = nullptr;

CurlNetworkManagerImpl::CurlNetworkManagerImpl(QObject *parent) : QThread(parent),
    bNeedFinish_(false)
  #if defined(Q_OS_MAC)
    , certPath_(QCoreApplication::applicationDirPath() + "/../resources/cert.pem")
  #elif defined (Q_OS_LINUX)
    , certPath_("/etc/windscribe/cert.pem")
  #endif
{

#ifdef MAKE_CURL_LOG_FILE
    logFilePath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    logFilePath_ += "/curl_log.txt";
    QFile::remove(logFilePath_);
    logFile_ = nullptr;
#endif

    multiHandle_ = curl_multi_init();
    nextId_ = 0;
    g_this = this;
    start(LowPriority);
}

CurlNetworkManagerImpl::~CurlNetworkManagerImpl()
{
    {
        QMutexLocker locker(&mutex_);
        bNeedFinish_ = true;
        waitCondition_.wakeAll();
    }
    curl_multi_wakeup(multiHandle_);
    wait();
    curl_multi_cleanup(multiHandle_);
}

size_t CurlNetworkManagerImpl::writeDataCallback(void *ptr, size_t size, size_t count, void *ri)
{
    RequestInfo *requestInfo = static_cast<RequestInfo *>(ri);
    QByteArray arr;
    arr.append((char*)ptr, size*count);
    emit g_this->requestNewData(requestInfo->id, arr);
    return size*count;
}

int CurlNetworkManagerImpl::progressCallback(void *ri,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow)
{
    RequestInfo *requestInfo = static_cast<RequestInfo *>(ri);
    if (dltotal > 0)
        emit g_this->requestProgress(requestInfo->id, dlnow, dltotal);
    return 0;
}

bool CurlNetworkManagerImpl::setupBasicOptions(RequestInfo *requestInfo, const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings)
{
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_WRITEFUNCTION, writeDataCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_WRITEDATA, requestInfo) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_URL, request.url().toString().toStdString().c_str()) != CURLE_OK) return false;

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CONNECTTIMEOUT_MS , request.timeout()) != CURLE_OK) return false;

    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_XFERINFOFUNCTION, progressCallback) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_XFERINFODATA, requestInfo) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_NOPROGRESS, 0) != CURLE_OK) return false;

    struct curl_slist *list = NULL;
    list = curl_slist_append(list, request.contentTypeHeader().toStdString().c_str());
    if (list == NULL) return false;
    requestInfo->curlLists << list;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_HTTPHEADER, list) != CURLE_OK) return false;

    if (!setupResolveHosts(requestInfo, request, ips)) return false;
    if (!setupSslVerification(requestInfo, request)) return false;
    if (!setupProxy(requestInfo, proxySettings)) return false;

    curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PRIVATE, new quint64(requestInfo->id));    // our user data, must be deleted in the RequestInfo destructor
    return true;
}

CURLcode CurlNetworkManagerImpl::sslctx_function(CURL *curl, void *sslctx, void *parm)
{
    Q_UNUSED(curl);

    X509_STORE *store = X509_STORE_new();
    SSL_CTX_set_cert_store((SSL_CTX *)sslctx, store);

    CertManager *certManager = static_cast<CertManager *>(parm);
    for (int i = 0; i < certManager->count(); ++i)
    {
        X509_STORE_add_cert(store, certManager->getCert(i));
    }

    return CURLE_OK;
}

quint64 CurlNetworkManagerImpl::get(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    RequestInfo *requestInfo = new RequestInfo();
    quint64 id = nextId_++;
    requestInfo->id = id;
    requestInfo->curlEasyHandle = curl_easy_init();

    if (requestInfo->curlEasyHandle)  {
        if (setupBasicOptions(requestInfo, request, ips, proxySettings)) {
            QMutexLocker locker(&mutex_);
            activeRequests_[id] = requestInfo;
            waitCondition_.wakeAll();
            curl_multi_wakeup(multiHandle_);
            return id;
        }
    }

    // here if something failed
    WS_ASSERT(false);
    delete requestInfo;
    emit requestFinished(id, CURLE_FAILED_INIT);       // this signal must be queued
    return id;
}

quint64 CurlNetworkManagerImpl::post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    RequestInfo *requestInfo = new RequestInfo();
    quint64 id = nextId_++;
    requestInfo->id = id;
    requestInfo->curlEasyHandle = curl_easy_init();

    if (requestInfo->curlEasyHandle) {
        if (setupBasicOptions(requestInfo, request, ips, proxySettings)) {
            // set additional post request options
            if ((curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_POSTFIELDSIZE, data.size()) == CURLE_OK) &&
               (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_COPYPOSTFIELDS, data.data()) == CURLE_OK)) {
                QMutexLocker locker(&mutex_);
                activeRequests_[id] = requestInfo;
                waitCondition_.wakeAll();
                curl_multi_wakeup(multiHandle_);
                return id;
            }
        }
    }
    // here if something failed
    WS_ASSERT(false);
    delete requestInfo;
    emit requestFinished(id, CURLE_FAILED_INIT);       // this signal must be queued
    return id;
}

quint64 CurlNetworkManagerImpl::put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    RequestInfo *requestInfo = new RequestInfo();
    quint64 id = nextId_++;
    requestInfo->id = id;
    requestInfo->curlEasyHandle = curl_easy_init();

    if (requestInfo->curlEasyHandle) {
        if (setupBasicOptions(requestInfo, request, ips, proxySettings)) {
            // set additional put request options
            if ((curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_POSTFIELDSIZE, data.size()) == CURLE_OK) &&
               (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_COPYPOSTFIELDS, data.data()) == CURLE_OK) &&
               (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CUSTOMREQUEST, "PUT") == CURLE_OK)) {
                QMutexLocker locker(&mutex_);
                activeRequests_[id] = requestInfo;
                waitCondition_.wakeAll();
                curl_multi_wakeup(multiHandle_);
                return id;
            }
        }
    }
    // here if something failed
    WS_ASSERT(false);
    delete requestInfo;
    emit requestFinished(id, CURLE_FAILED_INIT);       // this signal must be queued
    return id;
}

quint64 CurlNetworkManagerImpl::deleteResource(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    RequestInfo *requestInfo = new RequestInfo();
    quint64 id = nextId_++;
    requestInfo->id = id;
    requestInfo->curlEasyHandle = curl_easy_init();

    if (requestInfo->curlEasyHandle) {
        if (setupBasicOptions(requestInfo, request, ips, proxySettings)) {
            // set additional delete request options
            if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CUSTOMREQUEST, "DELETE") == CURLE_OK) {
                QMutexLocker locker(&mutex_);
                activeRequests_[id] = requestInfo;
                waitCondition_.wakeAll();
                curl_multi_wakeup(multiHandle_);
                return id;
            }
        }
    }
    // here if something failed
    WS_ASSERT(false);
    delete requestInfo;
    emit requestFinished(id, CURLE_FAILED_INIT);       // this signal must be queued
    return id;
}

void CurlNetworkManagerImpl::abort(quint64 replyId)
{
    QMutexLocker locker(&mutex_);
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end()) {
        it.value()->isNeedRemoveFromMultiHandle = true;
    }
}

void CurlNetworkManagerImpl::run()
{
    //BIND_CRASH_HANDLER_FOR_THREAD();

#ifdef MAKE_CURL_LOG_FILE
    logFile_ = fopen(logFilePath_.toStdString().c_str(), "w+");
#endif

    while (!bNeedFinish_) {

        {
            QMutexLocker locker(&mutex_);
            while (activeRequests_.isEmpty() && !bNeedFinish_)
                waitCondition_.wait(&mutex_);
        }

        if (bNeedFinish_)
            break;

        {
            QMutexLocker locker(&mutex_);
            // add curl handles for requests which have not been added
            for (auto it = activeRequests_.begin(); it != activeRequests_.end(); ++it) {
                if (!it.value()->isAddedToMultiHandle) {
                    curl_multi_add_handle(multiHandle_, it.value()->curlEasyHandle);
                    it.value()->isAddedToMultiHandle = true;
                }
            }

            // check for need to be deleted requests (for cases when the function abort() was called)
            for (auto it = activeRequests_.begin(); it != activeRequests_.end(); /*++it*/) {  // should not increment it in the for loop
                if (it.value()->isNeedRemoveFromMultiHandle) {
                    RequestInfo *ri = it.value();
                    if (it.value()->isAddedToMultiHandle)
                        curl_multi_remove_handle(multiHandle_, ri->curlEasyHandle);
                    it = activeRequests_.erase(it);
                    delete ri;
                } else {
                    ++it;
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
                  quint64 *pointerId;
                  curl_easy_getinfo(curlEasyHandle, CURLINFO_PRIVATE, &pointerId);
                  WS_ASSERT(pointerId != nullptr);

                  quint64 id = *pointerId;
                  auto it = activeRequests_.find(id);
                  WS_ASSERT(it != activeRequests_.end());
                  WS_ASSERT(it.value()->curlEasyHandle == curlEasyHandle);
                  emit requestFinished(id, curlMsg->data.result);

                  //remove request from activeRequests
                  curl_multi_remove_handle(multiHandle_, curlEasyHandle);
                  delete it.value();
                  activeRequests_.remove(id);
              }
            } while(curlMsg);
        }

        // wait for activity or timeout
        int numfds;
        curl_multi_poll(multiHandle_, NULL, 0, 1000, &numfds);
    }

    for (auto it = activeRequests_.begin(); it != activeRequests_.end(); ++it)
        delete it.value();
    activeRequests_.clear();

#ifdef MAKE_CURL_LOG_FILE
    fclose(logFile_);
#endif
}

bool CurlNetworkManagerImpl::setupResolveHosts(RequestInfo *requestInfo, const NetworkRequest &request, const QStringList &ips)
{
    if (!ips.isEmpty()) {
        QString strResolve = request.url().host() + ":443" + ":" + ips.join(",");
        struct curl_slist *hosts = curl_slist_append(NULL, strResolve.toStdString().c_str());
        if (hosts == NULL) return false;
        requestInfo->curlLists << hosts;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_RESOLVE, hosts) != CURLE_OK) return false;
    } else {
        WS_ASSERT(false);
    }
    return true;
}

bool CurlNetworkManagerImpl::setupSslVerification(RequestInfo *requestInfo, const NetworkRequest &request)
{
#if defined(Q_OS_MAC) || defined (Q_OS_LINUX)
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_CAINFO, certPath_.toStdString().c_str()) != CURLE_OK) return false;
#endif
    if (request.isIgnoreSslErrors())  {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 0) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYHOST, 0) != CURLE_OK) return false;
    } else  {
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_VERIFYPEER, 1) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_CTX_FUNCTION, *sslctx_function) != CURLE_OK) return false;
        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_SSL_CTX_DATA, &certManager_) != CURLE_OK) return false;
    }

#ifdef MAKE_CURL_LOG_FILE
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_VERBOSE, 1) != CURLE_OK) return false;
    if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_STDERR, logFile_) != CURLE_OK) return false;
#endif
    return true;
}

bool CurlNetworkManagerImpl::setupProxy(RequestInfo *requestInfo, const types::ProxySettings &proxySettings)
{
    QString proxyString;
    if (proxySettings.isProxyEnabled())
    {
        if (proxySettings.option() == PROXY_OPTION_NONE)
            return true;    //nothing todo
        else if (proxySettings.option() == PROXY_OPTION_HTTP)
            proxyString = "http://" + proxySettings.address() + ":" + QString::number(proxySettings.getPort());
        else if (proxySettings.option() == PROXY_OPTION_SOCKS)
            proxyString = "socks5://" + proxySettings.address() + ":" + QString::number(proxySettings.getPort());

        if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXY, proxyString.toStdString().c_str()) != CURLE_OK)
            return false;

        if (!proxySettings.getUsername().isEmpty())  {
            if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXYUSERNAME, proxySettings.getUsername().toStdString().c_str()) != CURLE_OK)
                return false;
        }
        if (!proxySettings.getPassword().isEmpty()) {
            if (curl_easy_setopt(requestInfo->curlEasyHandle, CURLOPT_PROXYPASSWORD, proxySettings.getPassword().toStdString().c_str()) != CURLE_OK)
                return false;
        }
    }
    return true;
}
