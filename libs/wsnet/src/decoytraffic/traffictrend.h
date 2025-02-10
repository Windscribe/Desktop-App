#pragma once

#include <chrono>

namespace wsnet {


// Helper class for TrafficTrend
enum class TrendType { kMin, kMax, kMinMax };
class Trend
{
public:
    explicit Trend(TrendType type);

    void start();
    bool isFinished() const;

    int uploadTries() const { return tries_; }
    int attemptsToIncrease() const { return attemptsToIncrease_; }
    void decrementAttemptsToIncrease() { attemptsToIncrease_--; }
    TrendType type() const { return type_; }
    void changeType(TrendType type) { type_ = type; }

private:
    TrendType type_;
    std::chrono::time_point<std::chrono::steady_clock> resetTime_;
    unsigned int tries_ = 0;
    unsigned int attemptsToIncrease_ = 1;

};

class TrafficTrend
{

public:
    explicit TrafficTrend();

    unsigned int calculateTraffic(int targetData, bool isUpload);
    void setUpperLimitMultiplier(int val) { upperLimitMultiplier_ = val; }

private:
    Trend currentUploadTrend_;
    Trend currentDownloadTrend_;
    int upperLimitMultiplier_ = 16;
    static constexpr int kLowerLimitMultiplier = 1;
    static constexpr int kMinMaxRandomPercentage = 90;
    static constexpr int kMinMaxPercentage = 25;

    int getAverage(Trend &trend);
};


} // namespace wsnet
