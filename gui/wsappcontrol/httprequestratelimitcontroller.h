#ifndef HTTPREQUESTRATELIMITCONTROLLER_H
#define HTTPREQUESTRATELIMITCONTROLLER_H

#include <mutex>
#include <vector>

class HttpRequestRateLimitController
{
public:
    bool isRightRequest();

private:
    const int PERIOD_MS = 1000;
    const size_t MAX_REQUEST_PER_PERIOD = 3;
	std::mutex mutex_;
    std::vector<int64_t > last3Times_;

    int64_t getCurTime();
};

#endif // HTTPREQUESTRATELIMITCONTROLLER_H
