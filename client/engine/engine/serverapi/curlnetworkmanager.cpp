#include "curlnetworkmanager.h"
#include <curl/curl.h>
#include <QMap>
#include <QDebug>
#include <QCoreApplication>
#include <openssl/ssl.h>
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QStandardPaths>

CurlNetworkManager::CurlNetworkManager(QObject *parent) : QThread(parent),
    bIgnoreSslErrors_(false), bNeedFinish_(false), bProxyEnabled_(true)
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

    start(LowPriority);
}

CurlNetworkManager::~CurlNetworkManager()
{
    bNeedFinish_ = true;
    waitCondition_.wakeAll();
    wait();
}

size_t write_to_bytearray(void *ptr, size_t size, size_t count, void *stream)
{
    QByteArray *arr = (QByteArray *)stream;
    arr->append((char*)ptr, size*count);
    return size*count;
}

CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm)
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

void CurlNetworkManager::get(CurlRequest *curlRequest, uint timeout, const QString &hostname, const QStringList &ips)
{
    curlRequest->setMethodType(CurlRequest::METHOD_GET);
    curlRequest->setTimeout(timeout);
    curlRequest->setHostname(hostname);
    curlRequest->setIps(ips);

    mutexQueue_.lock();
    queue_.enqueue(curlRequest);
    waitCondition_.wakeAll();
    mutexQueue_.unlock();
}

void CurlNetworkManager::post(CurlRequest *curlRequest, uint timeout, const QString &contentTypeHeader,
                              const QString &hostname, const QStringList &ips)
{
    curlRequest->setMethodType(CurlRequest::METHOD_POST);
    curlRequest->setTimeout(timeout);
    curlRequest->setContentTypeHeader(contentTypeHeader);
    curlRequest->setHostname(hostname);
    curlRequest->setIps(ips);

    mutexQueue_.lock();
    queue_.enqueue(curlRequest);
    waitCondition_.wakeAll();
    mutexQueue_.unlock();
}

void CurlNetworkManager::put(CurlRequest *curlRequest, uint timeout, const QString &contentTypeHeader, const QString &hostname, const QStringList &ips)
{
    curlRequest->setMethodType(CurlRequest::METHOD_PUT);
    curlRequest->setTimeout(timeout);
    curlRequest->setContentTypeHeader(contentTypeHeader);
    curlRequest->setHostname(hostname);
    curlRequest->setIps(ips);

    mutexQueue_.lock();
    queue_.enqueue(curlRequest);
    waitCondition_.wakeAll();
    mutexQueue_.unlock();
}

void CurlNetworkManager::deleteResource(CurlRequest *curlRequest, uint timeout, const QString &hostname, const QStringList &ips)
{
    curlRequest->setMethodType(CurlRequest::METHOD_DELETE);
    curlRequest->setTimeout(timeout);
    curlRequest->setHostname(hostname);
    curlRequest->setIps(ips);

    mutexQueue_.lock();
    queue_.enqueue(curlRequest);
    waitCondition_.wakeAll();
    mutexQueue_.unlock();
}

void CurlNetworkManager::setIgnoreSslErrors(bool bIgnore)
{
    QMutexLocker lock(&mutexAccess_);
    bIgnoreSslErrors_ = bIgnore;
}

bool CurlNetworkManager::isCurlSslError(CURLcode curlCode)
{
    if (curlCode == CURLE_SSL_CONNECT_ERROR || curlCode == CURLE_SSL_CERTPROBLEM || curlCode == CURLE_SSL_CIPHER ||
        curlCode == CURLE_SSL_ISSUER_ERROR || curlCode == CURLE_SSL_PINNEDPUBKEYNOTMATCH || curlCode == CURLE_SSL_INVALIDCERTSTATUS ||
        curlCode == CURLE_SSL_CACERT || curlCode == CURLE_PEER_FAILED_VERIFICATION)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CurlNetworkManager::setProxySettings(const ProxySettings &proxySettings)
{
    QMutexLocker lock(&mutexAccess_);
    proxySettings_ = proxySettings;
}

void CurlNetworkManager::setProxyEnabled(bool bEnabled)
{
    QMutexLocker lock(&mutexAccess_);
    bProxyEnabled_ = bEnabled;
}

void CurlNetworkManager::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    CURLM *multi_handle = curl_multi_init();
    int still_running = 0;
    QMap<CURL *, CurlRequest *> map;

#ifdef MAKE_CURL_LOG_FILE
    logFile_ = fopen(logFilePath_.toStdString().c_str(), "w+");
#endif

    while (true)
    {
        CurlRequest *request = NULL;

        mutexQueue_.lock();
        if (!queue_.isEmpty())
        {
            request = queue_.dequeue();
        }
        else
        {
            if (still_running == 0)
            {
                waitCondition_.wait(&mutexQueue_);
            }
        }
        mutexQueue_.unlock();

        still_running = 0;

        if (request)
        {
            CURL *curl = makeRequest(request);
            if (curl)
            {
                map[curl] = request;
                curl_multi_add_handle(multi_handle, curl);
                curl_multi_perform(multi_handle, &still_running);
            }
            else
            {
                Q_ASSERT(false);
                emit finished(request);
            }
        }
        else
        {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            curl_multi_perform(multi_handle, &still_running);
        }

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
                    CurlRequest *curlRequest = it.value();
                    // if we have another IPs for connect, try it
                    if (m->data.result != CURLE_OK && /*!isCurlSslError(m->data.result) &&*/ curlRequest->isHasNextIp())
                    {
                        //qDebug() << "===== Try another IP";
                        map.remove(e);
                        curl_multi_remove_handle(multi_handle, e);
                        curl_easy_cleanup(e);

                        CURL *curl = makeRequest(curlRequest);
                        if (curl)
                        {
                            map[curl] = curlRequest;
                            curl_multi_add_handle(multi_handle, curl);
                            still_running++;
                        }
                        else
                        {
                            qCDebug(LOG_CURL_MANAGER) << "Can't create curl object";
                            Q_ASSERT(false);
                            emit finished(curlRequest);
                        }
                    }
                    else
                    {
                        curl_off_t download;
                        if (curl_easy_getinfo(e, CURLINFO_SIZE_DOWNLOAD_T, &download) != CURLE_OK)
                        {
                            Q_ASSERT(false);
                        }
                        // check if gzip compression used
                        //Q_ASSERT(download < 30000);

                        curlRequest->setCurlRetCode(m->data.result);
                        emit finished(curlRequest);

                        map.remove(e);
                        curl_multi_remove_handle(multi_handle, e);
                        curl_easy_cleanup(e);
                    }
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
        delete it.value();
        curl_multi_remove_handle(multi_handle, it.key());
        curl_easy_cleanup(it.key());
    }
    map.clear();

    curl_multi_cleanup(multi_handle);

#ifdef MAKE_CURL_LOG_FILE
    fclose(logFile_);
#endif
}

CURL *CurlNetworkManager::makeRequest(CurlRequest *curlRequest)
{
    if (curlRequest->getMethodType() == CurlRequest::METHOD_GET)
    {
        return makeGetRequest(curlRequest);
    }
    else if (curlRequest->getMethodType() == CurlRequest::METHOD_POST)
    {
        return makePostRequest(curlRequest);
    }
    else if (curlRequest->getMethodType() == CurlRequest::METHOD_PUT)
    {
        return makePutRequest(curlRequest);
    }
    else if (curlRequest->getMethodType() == CurlRequest::METHOD_DELETE)
    {
        return makeDeleteRequest(curlRequest);
    }
    else
    {
        Q_ASSERT(false);
        return NULL;
    }
}

CURL *CurlNetworkManager::makeGetRequest(CurlRequest *curlRequest)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_bytearray) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, curlRequest->getAnswerPointer()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlRequest->getGetData().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS , curlRequest->getTimeout()) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlRequest, curl)) goto failed;
        if (!setupSslVerification(curl)) goto failed;
        if (!setupProxy(curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

CURL *CurlNetworkManager::makePostRequest(CurlRequest *curlRequest)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_bytearray) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK)  goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, curlRequest->getAnswerPointer()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlRequest->getUrl().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;

        struct curl_slist *list = NULL;
        list = curl_slist_append(list, curlRequest->getContentTypeHeader().toStdString().c_str());
        if (list == NULL) goto failed;
        curlRequest->addCurlListForFreeLater(list);

        if (curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, curlRequest->getPostData().size()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, curlRequest->getPostData().data()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS , curlRequest->getTimeout()) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlRequest, curl)) goto failed;
        if (!setupSslVerification(curl)) goto failed;
        if (!setupProxy(curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

CURL *CurlNetworkManager::makePutRequest(CurlRequest *curlRequest)
{
    // the same as making post, only add CURLOPT_CUSTOMREQUEST field
    CURL *curl = makePostRequest(curlRequest);
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

CURL *CurlNetworkManager::makeDeleteRequest(CurlRequest *curlRequest)
{
    CURL *curl = curl_easy_init();

    if (curl)
    {
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_bytearray) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, curlRequest->getAnswerPointer()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_URL, curlRequest->getGetData().toStdString().c_str()) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1) != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK) goto failed;
        if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, curlRequest->getTimeout()) != CURLE_OK) goto failed;

        if (!setupResolveHosts(curlRequest, curl)) goto failed;
        if (!setupSslVerification(curl)) goto failed;
        if (!setupProxy(curl)) goto failed;

        return curl;
    }

failed:
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

bool CurlNetworkManager::setupResolveHosts(CurlRequest *curlRequest, CURL *curl)
{
    if (curlRequest->isHasNextIp())
    {
        CURLSH *share_handle = curl_share_init();
        if (curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS) != CURLSHE_OK) return false;
        if (curl_easy_setopt(curl, CURLOPT_SHARE, share_handle) != CURLE_OK) return false;

        struct curl_slist *hosts = NULL;
        QString s = curlRequest->getHostname() + ":443" + ":" + curlRequest->getNextIp();
        hosts = curl_slist_append(NULL, s.toStdString().c_str());
        if (hosts == NULL) return false;

        curlRequest->addCurlListForFreeLater(hosts);
        if (curl_easy_setopt(curl, CURLOPT_RESOLVE, hosts) != CURLE_OK) return false;
    }
    return true;
}

bool CurlNetworkManager::setupSslVerification(CURL *curl)
{
    QMutexLocker lock(&mutexAccess_);
#if defined(Q_OS_MAC) || defined (Q_OS_LINUX)
    if (curl_easy_setopt(curl, CURLOPT_CAINFO, certPath_.toStdString().c_str()) != CURLE_OK) return false;
#endif
    if (bIgnoreSslErrors_)
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

bool CurlNetworkManager::setupProxy(CURL *curl)
{
    QMutexLocker lock(&mutexAccess_);
    QString proxyString;
    if (bProxyEnabled_)
    {
        if (proxySettings_.option() == PROXY_OPTION_NONE)
        {
            //nothing todo
            return true;
        }
        else if (proxySettings_.option() == PROXY_OPTION_HTTP)
        {
            proxyString = "http://" + proxySettings_.address() + ":" + QString::number(proxySettings_.getPort());
        }
        else if (proxySettings_.option() == PROXY_OPTION_SOCKS)
        {
            proxyString = "socks5://" + proxySettings_.address() + ":" + QString::number(proxySettings_.getPort());
        }

        if (curl_easy_setopt(curl, CURLOPT_PROXY, proxyString.toStdString().c_str()) != CURLE_OK) return false;
        if (!proxySettings_.getUsername().isEmpty())
        {
            if (curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxySettings_.getUsername().toStdString().c_str()) != CURLE_OK) return false;
        }
        if (!proxySettings_.getPassword().isEmpty())
        {
            if (curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, proxySettings_.getPassword().toStdString().c_str()) != CURLE_OK) return false;
        }
    }
    return true;
}
