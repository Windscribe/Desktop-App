#include "curlrequest.h"

CurlRequest::CurlRequest() : curlCode_(CURLE_FAILED_INIT)
{
}

CurlRequest::~CurlRequest()
{
    Q_FOREACH(struct curl_slist *list, curlLists_)
    {
        curl_slist_free_all(list);
    }
    Q_FOREACH(CURLSH *sh, curlShareHandles_)
    {
        curl_share_cleanup(sh);
    }
}

void CurlRequest::setGetData(const QString &data)
{
    getData_ = data;
}

QString CurlRequest::getGetData() const
{
    return getData_;
}

void CurlRequest::setPostData(const QByteArray &data)
{
    postData_ = data;
}

QByteArray CurlRequest::getPostData() const
{
    return postData_;
}

void CurlRequest::setUrl(const QString &strUrl)
{
    strUrl_ = strUrl;
}

QString CurlRequest::getUrl() const
{
    return strUrl_;
}

QByteArray CurlRequest::getAnswer() const
{
    return answer_;
}

const QByteArray *CurlRequest::getAnswerPointer() const
{
    return &answer_;
}

void CurlRequest::setCurlRetCode(CURLcode code)
{
    curlCode_ = code;
}

CURLcode CurlRequest::getCurlRetCode() const
{
    return curlCode_;
}

void CurlRequest::setMethodType(CurlRequest::MethodType type)
{
    methodType_ = type;
}

CurlRequest::MethodType CurlRequest::getMethodType() const
{
    return methodType_;
}

void CurlRequest::setTimeout(uint timeout)
{
    timeout_ = timeout;
}

uint CurlRequest::getTimeout() const
{
    return timeout_;
}

void CurlRequest::setContentTypeHeader(const QString &strHeader)
{
    contentTypeHeader_ = strHeader;
}

QString CurlRequest::getContentTypeHeader() const
{
    return contentTypeHeader_;
}

void CurlRequest::addCurlListForFreeLater(curl_slist *list)
{
    curlLists_ << list;
}

void CurlRequest::addCurlShareHandleForFreeLater(CURLSH *share_handle)
{
    curlShareHandles_ << share_handle;
}

void CurlRequest::setHostname(const QString &hostname)
{
    hostname_ = hostname;
}

QString CurlRequest::getHostname() const
{
    return hostname_;
}

void CurlRequest::setIps(const QStringList &ips)
{
    Q_FOREACH(const QString &ip, ips)
    {
        ips_.enqueue(ip);
    }
}

QString CurlRequest::getNextIp()
{
    if (!ips_.isEmpty())
    {
        return ips_.dequeue();
    }
    else
    {
        return QString();
    }
}

bool CurlRequest::isHasNextIp() const
{
    return !ips_.isEmpty();
}
