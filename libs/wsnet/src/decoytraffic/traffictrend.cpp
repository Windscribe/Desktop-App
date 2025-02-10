#include "traffictrend.h"
#include <assert.h>
#include "utils/utils.h"

namespace wsnet {

Trend::Trend(TrendType type) : type_(type)
{
}

void Trend::start()
{
    resetTime_ = std::chrono::steady_clock::now();
    if ( type_ == TrendType::kMin) {
        tries_ = utils::random(2, 60);
        attemptsToIncrease_ = utils::random(1, 10);
    } else if ( type_ == TrendType::kMax) {
        tries_ = utils::random(60, 180);
        attemptsToIncrease_ = utils::random(1, 8);
    } else {
        tries_ = 0;
        attemptsToIncrease_ = 1;
    }
}

bool Trend::isFinished() const
{
    auto elapsedMs = (unsigned int)utils::since(resetTime_).count();
    return elapsedMs > tries_ * 1000;
}

TrafficTrend::TrafficTrend() :
    currentUploadTrend_(TrendType::kMin),
    currentDownloadTrend_(TrendType::kMax)
{

}

unsigned int TrafficTrend::calculateTraffic(int targetData, bool isUpload)
{
    unsigned int average = isUpload ? getAverage(currentUploadTrend_) : getAverage(currentDownloadTrend_);
    int attemptsToIncrease = isUpload ? currentUploadTrend_.attemptsToIncrease() : currentDownloadTrend_.attemptsToIncrease();
    unsigned int finalDataBasedOnAverage = targetData * average;

    if (attemptsToIncrease <= 2)
        return finalDataBasedOnAverage;

    if(isUpload)
        currentUploadTrend_.decrementAttemptsToIncrease();
    else
        currentDownloadTrend_.decrementAttemptsToIncrease();

    unsigned int chunk = finalDataBasedOnAverage / attemptsToIncrease;
    if (chunk > finalDataBasedOnAverage)
        return finalDataBasedOnAverage;
    else
        return chunk;
}

int TrafficTrend::getAverage(Trend &trend)
{
    if (trend.isFinished()) {
        int random = utils::random(0, 100);
        if (random <= kMinMaxRandomPercentage) {
            int minOrMax = utils::random(0, 100);
            if (minOrMax >= kMinMaxPercentage) {
                trend.changeType(TrendType::kMax);
                trend.start();
                return upperLimitMultiplier_;
            } else {
                trend.changeType(TrendType::kMin);
                trend.start();
                return kLowerLimitMultiplier;
            }
        } else {
            trend.changeType(TrendType::kMinMax);
            return utils::random(kLowerLimitMultiplier, upperLimitMultiplier_);
        }
    } else {
        if (trend.type() == TrendType::kMin)
            return kLowerLimitMultiplier;
        else if (trend.type() == TrendType::kMax)
            return upperLimitMultiplier_;
        else
            return utils::random(kLowerLimitMultiplier, upperLimitMultiplier_);
    }
}



} // namespace wsnet
