#ifndef CURLREQUEST_H
#define CURLREQUEST_H

#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVector>
#include <curl/curl.h>
#include "../Types/types.h"
#include "../Types/protocoltype.h"

class CurlRequest
{
public:
    CurlRequest();
    virtual ~CurlRequest();

    void setGetData(const QString &data);
    QString getGetData() const;

    void setPostData(const QByteArray &data);
    QByteArray getPostData() const;

    void setUrl(const QString &strUrl);
    QString getUrl() const;

    QByteArray getAnswer() const;
    const QByteArray *getAnswerPointer() const;

    void setCurlRetCode(CURLcode code);
    CURLcode getCurlRetCode() const;

    enum MethodType { METHOD_GET, METHOD_POST, METHOD_PUT, METHOD_DELETE };

    void setMethodType(MethodType type);
    MethodType getMethodType() const;

    void setTimeout(uint timeout);
    uint getTimeout() const;

    void setContentTypeHeader(const QString &strHeader);
    QString getContentTypeHeader() const;

    void addCurlListForFreeLater(struct curl_slist *list);
    void addCurlShareHandleForFreeLater(CURLSH *share_handle);

    void setHostname(const QString &hostname);
    QString getHostname() const;

    void setIps(const QStringList &ips);
    QString getNextIp();
    bool isHasNextIp() const;

private:
    QString getData_;
    QByteArray postData_;
    QByteArray answer_;
    CURLcode curlCode_;
    QString strUrl_;
    MethodType methodType_;
    uint timeout_;
    QString contentTypeHeader_;
    QVector<struct curl_slist *> curlLists_;
    QVector<CURLSH *> curlShareHandles_;
    QString hostname_;
    QQueue<QString> ips_;
};

#endif // CURLREQUEST_H
