#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "WSNetCancelableCallback.h"

namespace wsnet {

// A class allowing to cancel an async callback thread-safely
template <class F>
class CancelableCallback : public WSNetCancelableCallback
{
public:
    CancelableCallback(F func) : func_(func) {}

    void cancel() override
    {
        std::scoped_lock locker(mutex_);
        isCanceled_ = true;
    }

    bool isCanceled()
    {
        std::scoped_lock locker(mutex_);
        return isCanceled_;
    }

    template <typename... Args> void call(Args&&... args)
    {
        std::scoped_lock locker(mutex_);
        if (!isCanceled_)
            func_(std::forward<Args>(args)...);
    }

private:
    std::recursive_mutex mutex_;
    F func_;
    bool isCanceled_ = false;

};

// for 3 callbacks (for HttpManager)
template <class F1, class F2, class F3>
class CancelableCallback3 : public WSNetCancelableCallback
{
public:
    CancelableCallback3(F1 func1, F2 func2, F3 func3) : func1_(func1), func2_(func2), func3_(func3) {}

    void cancel() override
    {
        std::scoped_lock locker(mutex_);
        isCanceled_ = true;
    }

    bool isCanceled()
    {
        std::scoped_lock locker(mutex_);
        return isCanceled_;
    }

    template <typename... Args> void callFinished(Args&&... args)
    {
        std::scoped_lock locker(mutex_);
        if (!isCanceled_ && func1_)
            func1_(std::forward<Args>(args)...);
    }

    template <typename... Args> void callProgress(Args&&... args)
    {
        std::scoped_lock locker(mutex_);
        if (!isCanceled_ && func2_)
            func2_(std::forward<Args>(args)...);
    }

    template <typename... Args> void callDataReady(Args&&... args)
    {
        std::scoped_lock locker(mutex_);
        if (!isCanceled_ && func3_)
            func3_(std::forward<Args>(args)...);
    }

    bool isDataReadyNull() const  { return func3_ == nullptr; }

private:
    std::recursive_mutex mutex_;
    F1 func1_;
    F2 func2_;
    F3 func3_;
    bool isCanceled_ = false;

};

} // namespace wsnet
