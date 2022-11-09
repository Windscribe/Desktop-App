#include "networkreply.h"
#include "curlreply.h"
#include "networkaccessmanager.h"
#include "utils/ws_assert.h"

NetworkReply::NetworkReply(NetworkAccessManager *parent) : QObject(parent), curlReply_(nullptr), manager_(parent), error_(NetworkReply::NoError)
{

}

NetworkReply::~NetworkReply()
{
    abort();
}

void NetworkReply::abort()
{
    manager_->abort(this);
    if (curlReply_) {
        delete curlReply_;
        curlReply_ = nullptr;
    }
}

QByteArray NetworkReply::readAll()
{
    if (curlReply_)
    {
        return curlReply_->readAll();
    }
    else
    {
        WS_ASSERT(false);
        return QByteArray();
    }
}

NetworkReply::NetworkError NetworkReply::error() const
{
    return error_;
}

QString NetworkReply::errorString() const
{
    return errorString_;
}

bool NetworkReply::isSuccess() const
{
    return error_ == NoError;
}


void NetworkReply::setCurlReply(CurlReply *curlReply)
{
    curlReply_ = curlReply;
}

void NetworkReply::abortCurl()
{
    if (curlReply_)
    {
        curlReply_->abort();
    }
}

void NetworkReply::checkForCurlError()
{
    if (curlReply_ && !curlReply_->isSuccess())
    {
        error_ = CurlError;
        errorString_ = curlReply_->errorString();
    }
}

void NetworkReply::setError(NetworkReply::NetworkError err)
{
    error_ = err;
    if (err == NoError) {
        errorString_ = "NoError";
    } else if (err == TimeoutExceed) {
        errorString_ = "TimeoutExceed";
    } else if (err == DnsResolveError) {
        errorString_ = "DnsResolveError";
    } else if (err == CurlError) {
        errorString_ = "CurlError";
    } else {
        WS_ASSERT(false);
    }
}
