#ifndef CURLNETWORKMANAGER_H
#define CURLNETWORKMANAGER_H

#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include "curlinitcontroller.h"
#include "curlreply.h"
#include "certmanager.h"
#include "networkrequest.h"


// comment, if no need log file from curl
//#define MAKE_CURL_LOG_FILE      1

class CurlNetworkManager : public QThread
{
    Q_OBJECT
public:
    explicit CurlNetworkManager(QObject *parent = 0);
    virtual ~CurlNetworkManager();
    CurlReply *get(const NetworkRequest &request, const QStringList &ips);
    CurlReply *post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips);
    CurlReply *put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips);
    CurlReply *deleteResource(const NetworkRequest &request, const QStringList &ips);

    void abort(CurlReply *reply);

protected:
    virtual void run();

private slots:
    void handleRequest(quint64 id);

private:
    CurlInitController curlInit_;
    CertManager certManager_;
    QQueue<quint64> queue_;
    QWaitCondition waitCondition_;
    bool bNeedFinish_;
    QMap<quint64, CurlReply *> activeRequests_;
    QMutex mutex_;

#ifdef Q_OS_MAC
    QString certPath_;
#endif

#ifdef MAKE_CURL_LOG_FILE
    QString logFilePath_;
    FILE *logFile_;
#endif

    QMap<quint64, QSharedPointer<quint64> > idsMap_;   // need for curl callback functions, we pass pointer to quint64

    CurlReply *invokeRequest(CurlReply::REQUEST_TYPE type, const NetworkRequest &request, const QStringList &ips, const QByteArray &data = QByteArray());

    void setIdIntoMap(quint64 id);
    CURL *makeRequest(CurlReply *curlReply);
    CURL *makeGetRequest(CurlReply *curlReply);
    CURL *makePostRequest(CurlReply *curlReply);
    CURL *makePutRequest(CurlReply *curlReply);
    CURL *makeDeleteRequest(CurlReply *curlReply);

    bool setupResolveHosts(CurlReply *curlReply, CURL *curl);
    bool setupSslVerification(CurlReply *curlReply, CURL *curl);
    bool setupProxy(CurlReply *curlReply, CURL *curl);

    static size_t writeDataCallback(void *ptr, size_t size, size_t count, void *id);
    static int progressCallback(void *id,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow);
};

#endif // CURLNETWORKMANAGER_H
