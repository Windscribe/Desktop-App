#include "httprequestratelimitcontroller.h"
#include "types.h"
#include <assert.h>

bool HttpRequestRateLimitController::isRightRequest()
{
	mutex_.lock();
    bool bRet = false;
    if (last3Times_.size() < MAX_REQUEST_PER_PERIOD)
    {
		last3Times_.push_back(getCurTime());
        bRet = true;
    }
    else
    {
        assert(last3Times_.size() == MAX_REQUEST_PER_PERIOD);
        __int64_t curTime = getCurTime();
        if ((curTime - last3Times_.at(0)) > PERIOD_MS)
        {
            bRet = true;
			last3Times_.erase(last3Times_.begin());
            last3Times_.push_back(curTime);
        }
    }

	mutex_.unlock();
    return bRet;
}

__int64_t HttpRequestRateLimitController::getCurTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
