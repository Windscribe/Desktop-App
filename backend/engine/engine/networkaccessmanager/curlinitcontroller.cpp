#include "curlinitcontroller.h"
#include <curl/curl.h>

CurlInitController::CurlInitController()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlInitController::~CurlInitController()
{
    curl_global_cleanup();
}

void CurlInitController::init()
{
}
