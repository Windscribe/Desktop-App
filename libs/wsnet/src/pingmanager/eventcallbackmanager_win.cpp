#include "eventcallbackmanager_win.h"
#include <assert.h>

namespace wsnet {

EventCallbackManager_win::EventCallbackManager_win()
{
    hEvent_ = CreateEvent(NULL, false, false, NULL);
    thread_ = std::thread(std::bind(&EventCallbackManager_win::run, this));
}

EventCallbackManager_win::~EventCallbackManager_win()
{
    if (!isStopped_)
        stop();
}

void EventCallbackManager_win::stop()
{
    finish_ = true;
    SetEvent(hEvent_);
    thread_.join();
    CloseHandle(hEvent_);
    isStopped_ = true;
}

void EventCallbackManager_win::add(HANDLE hEvent, EventCallbackFunc callback)
{
    std::lock_guard locker(mutex_);
    assert(events_.find(hEvent) == events_.end());
    events_[hEvent] = callback;
    SetEvent(hEvent_);
}

void EventCallbackManager_win::remove(HANDLE hEvent)
{
    std::lock_guard locker(mutex_);
    auto it = events_.find(hEvent);
    if (it != events_.end()) {
        events_.erase(it);
        SetEvent(hEvent_);
    }
}


void EventCallbackManager_win::run()
{
    while (!finish_) {

        std::vector<HANDLE> handles;
        handles.push_back(hEvent_);

        {
            std::unique_lock<std::mutex> locker(mutex_);
            for (const auto &it : events_) {
                handles.push_back(it.first);
            }
        }

        WaitForMultipleObjects(handles.size(), &handles[0], false, INFINITE);

        if (finish_)
            return;

        {
            std::unique_lock<std::mutex> locker(mutex_);
            auto it = events_.begin();
            while(it != events_.end()) {
                if (WaitForSingleObject(it->first, 0) == WAIT_OBJECT_0) {
                    it->second();       // call callback
                    it = events_.erase(it);
                } else {
                    it++;
                }
            }
        }
    }
}


} // namespace wsnet
