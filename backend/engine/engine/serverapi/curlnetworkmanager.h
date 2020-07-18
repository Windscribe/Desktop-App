#ifndef CURLNETWORKMANAGER_H
#define CURLNETWORKMANAGER_H

#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include "curlrequest.h"
#include "certmanager.h"
#include "engine/proxy/proxysettings.h"

// comment, if no need log file from curl
//#define MAKE_CURL_LOG_FILE      1


class CurlNetworkManager : public QThread
{
    Q_OBJECT
public:
    explicit CurlNetworkManager(QObject *parent = 0);
    virtual ~CurlNetworkManager();
    void get(CurlRequest *curlRequest, uint timeout, const QString &hostname, const QStringList &ips);
    void post(CurlRequest *curlRequest, uint timeout, const QString &contentTypeHeader, const QString &hostname, const QStringList &ips);
    void put(CurlRequest *curlRequest, uint timeout, const QString &contentTypeHeader, const QString &hostname, const QStringList &ips);
    void deleteResource(CurlRequest *curlRequest, uint timeout, const QString &hostname, const QStringList &ips);

    void setIgnoreSslErrors(bool bIgnore);

    bool isCurlSslError(CURLcode curlCode);

    void setProxySettings(const ProxySettings &proxySettings);
    void setProxyEnabled(bool bEnabled);


signals:
    void finished(CurlRequest *curlRequest);

protected:
    virtual void run();

private:
    CertManager certManager_;
    bool bIgnoreSslErrors_;
    QQueue<CurlRequest *> queue_;
    QMutex mutexQueue_;
    QWaitCondition waitCondition_;
    bool bNeedFinish_;
    ProxySettings proxySettings_;
    bool bProxyEnabled_;

    QMutex mutexAccess_;

#ifdef Q_OS_MAC
    QString certPath_;
#endif

#ifdef MAKE_CURL_LOG_FILE
    QString logFilePath_;
    FILE *logFile_;
#endif

    CURL *makeRequest(CurlRequest *curlRequest);
    CURL *makeGetRequest(CurlRequest *curlRequest);
    CURL *makePostRequest(CurlRequest *curlRequest);
    CURL *makePutRequest(CurlRequest *curlRequest);
    CURL *makeDeleteRequest(CurlRequest *curlRequest);

    void setupResolveHosts(CurlRequest *curlRequest, CURL *curl);
    void setupSslVerification(CURL *curl);
    void setupProxy(CURL *curl);
};

#endif // CURLNETWORKMANAGER_H
