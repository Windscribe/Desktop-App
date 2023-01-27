#include "curlnetworkmanager2.h"
#include <curl/curl.h>
#include <QMap>
#include <QDebug>
#include <QCoreApplication>
#include <openssl/ssl.h>
//#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QStandardPaths>

CurlNetworkManager2 *g_this = nullptr;

CurlNetworkManager2::CurlNetworkManager2(QObject *parent) : QThread(parent),
    bNeedFinish_(false)
  #if defined(Q_OS_MAC)
    , certPath_(QCoreApplication::applicationDirPath() + "/../resources/cert.pem")
  #elif defined (Q_OS_LINUX)
    , certPath_("/etc/windscribe/cert.pem")
  #endif
{

#ifdef MAKE_CURL_LOG_FILE
    logFilePath_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    logFilePath_ += "/curl_log.txt";
    QFile::remove(logFilePath_);
    logFile_ = nullptr;
#endif

    g_this = this;

    start(LowPriority);
}

CurlNetworkManager2::~CurlNetworkManager2()
{
    bNeedFinish_ = true;
    waitCondition_.wakeAll();
    wait();
}

size_t CurlNetworkManager2::writeDataCallback(void *ptr, size_t size, size_t count, void *id)
{
    QMutexLocker locker(&g_this->mutex_);

    quint64 *requestId = static_cast<quint64 *>(id);
    auto it = g_this->activeRequests_.find(*requestId);
    if (it != g_this->activeRequests_.end())
    {
        QByteArray arr;
        arr.append((char*)ptr, size*count);
        it.value()->appendNewData(arr);
        emit it.value()->readyRead();
    }

    return size*count;
}

int CurlNetworkManager2::progressCallback(void *id,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow)
{
    Q_UNUSED(ultotal);
    Q_UNUSED(ulnow);

    QMutexLocker locker(&g_this->mutex_);

    quint64 *requestId = static_cast<quint64 *>(id);
    if (dltotal > 0)
    {
        auto it = g_this->activeRequests_.find(*requestId);
        if (it != g_this->activeRequests_.end())
        {
            emit it.value()->progress(dlnow, dltotal);
        }
    }

    return 0;
}

CURLcode CurlNetworkManager2::sslctx_function(CURL *curl, void *sslctx, void *parm)
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

CurlReply *CurlNetworkManager2::get(const NetworkRequest &request, const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    return invokeRequest(CurlReply::REQUEST_GET, request, ips);
}

CurlReply *CurlNetworkManager2::post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    return invokeRequest(CurlReply::REQUEST_POST, request, ips, data);
}

CurlReply *CurlNetworkManager2::put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    return invokeRequest(CurlReply::REQUEST_PUT, request, ips, data);
}

CurlReply *CurlNetworkManager2::deleteResource(const NetworkRequest &request, const QStringList &ips)
{
    QMutexLocker locker(&mutex_);
    return invokeRequest(CurlReply::REQUEST_DELETE, request, ips);
}

void CurlNetworkManager2::abort(CurlReply *reply)
{
    QMutexLocker lock(&mutex_);
    activeRequests_.remove(reply->id());
    idsMap_.remove(reply->id());
}

void CurlNetworkManager2::run()
{
    //BIND_CRASH_HANDLER_FOR_THREAD();
    CURLM *multi_handle = curl_multi_init();
    int still_running = 0;
    QMap<CURL *, quint64> map;

#ifdef MAKE_CURL_LOG_FILE
    logFile_ = fopen(logFilePath_.toStdString().c_str(), "w+");
#endif

    while (true)
    {
        mutex_.lock();
        while (still_running == 0 && queue_.isEmpty() && !bNeedFinish_)
        {
            waitCondition_.wait(&mutex_);
        }
        mutex_.unlock();

        if (bNeedFinish_)
        {
            break;
        }

        mutex_.lock();
        bool isExistRequest = false;
        quint64 id;
        if (!queue_.isEmpty())
        {
            id = queue_.dequeue();
            isExistRequest = true;
        }
        mutex_.unlock();


        still_running = 0;

        if (isExistRequest)
        {
            CURL *curl = nullptr;
            bool isMakeRequestCalled = false;
            {
                QMutexLocker locker(&mutex_);
                auto request = activeRequests_.find(id);
                if (request != activeRequests_.end())
                {
                    curl = makeRequest(request.value());
                    isMakeRequestCalled = true;
                }
            }
            if (curl)
            {
                map[curl] = id;
                curl_multi_add_handle(multi_handle, curl);
            }
            else
            {
                QMutexLocker locker(&mutex_);
                if (isMakeRequestCalled)
                {
                    auto request = activeRequests_.find(id);
                    if (request != activeRequests_.end())
                    {
                        request.value()->setCurlErrorCode(CURLE_FAILED_INIT);
                        emit request.value()->finished();
                        activeRequests_.erase(request);
                    }
                }
            }
        }
        else
        {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
        }

        // check for aborted requests
        {
            for (auto it = map.begin(); it != map.end(); /*++it*/) // should not increment it in the for loop
            {
                bool bFound = false;
                {
                    QMutexLocker locker(&mutex_);
                    auto findIt = activeRequests_.find(it.value());
                    bFound = findIt != activeRequests_.end();
                }
                if (!bFound)
                {
                    curl_multi_remove_handle(multi_handle, it.key());
                    curl_easy_cleanup(it.key());
                    it = map.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        curl_multi_perform(multi_handle, &still_running);

        // check finished requests
        struct CURLMsg *m;
        do
        {
            int msgq = 0;
            m = curl_multi_info_read(multi_handle, &msgq);
            if (m && (m->msg == CURLMSG_DONE))
            {
                CURL *e = m->easy_handle;

                auto it = map.find(e);
                if (it != map.end())
                {
                        {
                            QMutexLocker locker(&mutex_);
                            auto request = activeRequests_.find(it.value());
                            if (request != activeRequests_.end())
                            {
                                request.value()->setCurlErrorCode(m->data.result);
                                emit request.value()->finished();
                                activeRequests_.erase(request);
                            }
                        }

                        map.remove(e);

                    curl_multi_remove_handle(multi_handle, e);
                    curl_easy_cleanup(e);

                }
                else
                {
                    Q_ASSERT(false);
                }
            }
        } while(m);

        if (bNeedFinish_)
        {
            break;
        }
    }

    // delete not finished requests
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        //delete it.value();
        curl_multi_remove_handle(multi_handle, it.key());
        curl_easy_cleanup(it.key());
    }
    {
        QMutexLocker locker(&mutex_);
        activeRequests_.clear();
        idsMap_.clear();
    }
    map.clear();

    curl_multi_cleanup(multi_handle);

#ifdef MAKE_CURL_LOG_FILE
    fclose(logFile_);
#endif
}

void CurlNetworkManager2::handleRequest(quint64 id)
{
    QMutexLocker lock(&mutex_);
    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end())
    {
        queue_.enqueue(id);
        waitCondition_.wakeAll();
    }
}

CurlReply *CurlNetworkManager2::invokeRequest(CurlReply::REQUEST_TYPE type, const NetworkRequest &request, const QStringList &ips, const QByteArray &data /*= QByteArray*/)
{
    CurlReply *reply = new CurlReply(this, request, ips, type, data, this);
    Q_ASSERT(!activeRequests_.contains(reply->id()));
    activeRequests_[reply->id()] = reply;
    QMetaObject::invokeMethod(this, "handleRequest", Qt::QueuedConnection, Q_ARG(quint64, reply->id()));
    return reply;
}

void CurlNetworkManager2::setIdIntoMap(quint64 id)
{
    auto it = idsMap_.find(id);
    if (it == idsMap_.end())
    {
        idsMap_[id] = QSharedPointer<quint64>(new quint64(id));
    }
    else
    {
        *it.value() = id;
    }
}

CURL *CurlNetworkManager2::makeRequest(CurlReply *curlReply)
{
    if (curlReply->requestType() == CurlReply::REQUEST_GET)
    {
        return makeGetRequest(curlReply);
    }
    else if (curlReply->requestType() == CurlReply::REQUEST_POST)
    {
        return makePostRequest(curlReply);
    }
    else if (curlReply->requestType() == CurlReply::REQUEST_PUT)
    {
        return makePutRequest(curlReply);
    }
    else if (curlReply->requestType() == CurlReply::REQUEST_DELETE)
    {
        return makeDeleteRequest(curlReply);
    }
    else
    {
        Q_ASSERT(false);
        return NULL;
    }
}

CURL *CurlNetworkManager2::makeGetRequest(CurlReply *curlReply)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        setIdIntoMap(curlReply->id());

        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeDataCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlReply->networkRequest().url().toString().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS , curlReply->networkRequest().timeout()) != CURLE_OK) goto failed;

        if (curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_XFERINFODATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlReply, curl)) goto failed;
        if (!setupSslVerification(curlReply, curl)) goto failed;
        if (!setupProxy(curlReply, curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

CURL *CurlNetworkManager2::makePostRequest(CurlReply *curlReply)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        setIdIntoMap(curlReply->id());

        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeDataCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK)  goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlReply->networkRequest().url().toString().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;

        struct curl_slist *list = NULL;
        list = curl_slist_append(list, curlReply->networkRequest().contentTypeHeader().toStdString().c_str());
        if (list == NULL) goto failed;
        curlReply->addCurlListForFreeLater(list);

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlReply->postData().size()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, curlReply->postData().data()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS , curlReply->networkRequest().timeout()) != CURLE_OK) goto failed;

        if (curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_XFERINFODATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlReply, curl)) goto failed;
        if (!setupSslVerification(curlReply, curl)) goto failed;
        if (!setupProxy(curlReply, curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

CURL *CurlNetworkManager2::makePutRequest(CurlReply *curlReply)
{
    // the same as making post, only add CURLOPT_CUSTOMREQUEST field
    CURL *curl = makePostRequest(curlReply);
    if (curl)
    {
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT") != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            return NULL;
        }
        return curl;
    }
    else
    {
        return NULL;
    }
}

CURL *CurlNetworkManager2::makeDeleteRequest(CurlReply *curlReply)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        setIdIntoMap(curlReply->id());

        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeDataCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;

        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlReply->networkRequest().url().toString().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS , curlReply->networkRequest().timeout()) != CURLE_OK) goto failed;

        if (curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_XFERINFODATA, idsMap_[curlReply->id()].get()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlReply, curl)) goto failed;
        if (!setupSslVerification(curlReply, curl)) goto failed;
        if (!setupProxy(curlReply, curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

bool CurlNetworkManager2::setupResolveHosts(CurlReply *curlReply, CURL *curl)
{
    if (!curlReply->ips().isEmpty())
    {
        QString strResolve = curlReply->networkRequest().url().host() + ":443" + ":" + curlReply->ips().join(",");
        struct curl_slist *hosts = curl_slist_append(NULL, strResolve.toStdString().c_str());
        if (hosts == NULL) return false;
        curlReply->addCurlListForFreeLater(hosts);
        if (curl_easy_setopt(curl, CURLOPT_RESOLVE, hosts) != CURLE_OK) return false;
    }
    else
    {
        Q_ASSERT(false);
    }
    return true;
}

bool CurlNetworkManager2::setupSslVerification(CurlReply *curlReply, CURL *curl)
{
#if defined(Q_OS_MAC) || defined (Q_OS_LINUX)
    if (curl_easy_setopt(curl, CURLOPT_CAINFO, certPath_.toStdString().c_str()) != CURLE_OK) return false;
#endif
    if (curlReply->networkRequest().isIgnoreSslErrors())
    {
        if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0) != CURLE_OK) return false;
        if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0) != CURLE_OK) return false;
    }
    else
    {
        if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1) != CURLE_OK) return false;
        if (curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, *sslctx_function) != CURLE_OK) return false;
        if (curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, &certManager_) != CURLE_OK) return false;
    }

#ifdef MAKE_CURL_LOG_FILE
    if (curl_easy_setopt(curl, CURLOPT_VERBOSE, 1) != CURLE_OK) return false;
    if (curl_easy_setopt(curl, CURLOPT_STDERR, logFile_) != CURLE_OK) return false;
#endif
    return true;
}

bool CurlNetworkManager2::setupProxy(CurlReply *curlReply, CURL *curl)
{
    QString proxyString;
    if (curlReply->networkRequest().proxySettings().isProxyEnabled())
    {
        if (curlReply->networkRequest().proxySettings().option() == PROXY_OPTION_NONE)
        {
            //nothing todo
            return true;
        }
        else if (curlReply->networkRequest().proxySettings().option() == PROXY_OPTION_HTTP)
        {
            proxyString = "http://" + curlReply->networkRequest().proxySettings().address() + ":" + QString::number(curlReply->networkRequest().proxySettings().getPort());
        }
        else if (curlReply->networkRequest().proxySettings().option() == PROXY_OPTION_SOCKS)
        {
            proxyString = "socks5://" + curlReply->networkRequest().proxySettings().address() + ":" + QString::number(curlReply->networkRequest().proxySettings().getPort());
        }

        if (curl_easy_setopt(curl, CURLOPT_PROXY, proxyString.toStdString().c_str()) != CURLE_OK) return false;
        if (!curlReply->networkRequest().proxySettings().getUsername().isEmpty())
        {
            if (curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, curlReply->networkRequest().proxySettings().getUsername().toStdString().c_str()) != CURLE_OK) return false;
        }
        if (!curlReply->networkRequest().proxySettings().getPassword().isEmpty())
        {
            if (curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, curlReply->networkRequest().proxySettings().getPassword().toStdString().c_str()) != CURLE_OK) return false;
        }
    }
    return true;
}

