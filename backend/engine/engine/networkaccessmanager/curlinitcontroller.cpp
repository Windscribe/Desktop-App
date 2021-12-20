#include "curlinitcontroller.h"
#include <curl/curl.h>
#include "utils/logger.h"

bool CurlInitController::isInitialized_ = false;
QMutex CurlInitController::mutex_;

CurlInitController::CurlInitController()
{
    QMutexLocker locker(&mutex_);
    if (!isInitialized_)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        isInitialized_ = true;
        qCDebug(LOG_BASIC) << "Curl version:" << curl_version();
    }
}

CurlInitController::~CurlInitController()
{
    QMutexLocker locker(&mutex_);
    if (isInitialized_)
    {
        curl_global_cleanup();
        isInitialized_ = false;
    }
}
