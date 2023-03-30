#pragma once

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <curl/curl.h>

#include "certmanager.h"
#include "curlinitcontroller.h"
#include "networkrequest.h"
#include "types/proxysettings.h"


// comment, if no need log file from curl
//#define MAKE_CURL_LOG_FILE      1

// Implementing queries with curl library. Don't use it directly, use NetworkAccessManager instead.
class CurlNetworkManagerImpl : public QThread
{
    Q_OBJECT
public:
    explicit CurlNetworkManagerImpl(QObject *parent = 0);
    virtual ~CurlNetworkManagerImpl();

    // return unique request id which can be used for the abort function and in processing slots
    quint64 get(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings = types::ProxySettings());
    quint64 post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings = types::ProxySettings());
    quint64 put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings  = types::ProxySettings());
    quint64 deleteResource(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings  = types::ProxySettings());

    void abort(quint64 requestId);

protected:
    virtual void run();

signals:
    // this signal must be queued connected
    void requestFinished(quint64 requestId, CURLcode curlErrorCode);
    void requestProgress(quint64 requestId, qint64 bytesReceived, qint64 bytesTotal);
    void requestNewData(quint64 requestId, const QByteArray &newData);

private:
    CurlInitController curlInit_;
    CertManager certManager_;
    std::atomic<bool> bNeedFinish_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    std::atomic<quint64> nextId_;

#if defined(Q_OS_MAC) || defined (Q_OS_LINUX)
    QString certPath_;
#endif

#ifdef MAKE_CURL_LOG_FILE
    QString logFilePath_;
    FILE *logFile_;
#endif

    struct RequestInfo {
        quint64 id;
        CURL *curlEasyHandle = nullptr;
        QVector<struct curl_slist *> curlLists;
        bool isAddedToMultiHandle = false;
        bool isNeedRemoveFromMultiHandle = false;

        // free all curl handles and data
        ~RequestInfo() {
            if (curlEasyHandle) {
                // delete our user data (pointer to quint64)
                quint64 *id;
                if (curl_easy_getinfo(curlEasyHandle, CURLINFO_PRIVATE, &id) == CURLE_OK && id != nullptr)
                    delete id;
                curl_easy_cleanup(curlEasyHandle);
            }
            for (struct curl_slist *list : qAsConst(curlLists))
                curl_slist_free_all(list);
        }
    };

    CURLM *multiHandle_;
    QHash<quint64, RequestInfo *> activeRequests_;

    bool setupBasicOptions(RequestInfo *requestInfo, const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings);
    bool setupResolveHosts(RequestInfo *requestInfo, const NetworkRequest &request, const QStringList &ips);
    bool setupSslVerification(RequestInfo *requestInfo, const NetworkRequest &request);
    bool setupProxy(RequestInfo *requestInfo, const types::ProxySettings &proxySettings);

    static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm);
    static size_t writeDataCallback(void *ptr, size_t size, size_t count, void *ri);
    static int progressCallback(void *ri,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow);
};

