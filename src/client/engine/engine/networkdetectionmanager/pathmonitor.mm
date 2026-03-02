#include "pathmonitor.h"
#include "utils/log/categories.h"

PathMonitor::PathMonitor(): curIsOnline_(true), debounceTimer_(nullptr)
{
  monitor_ = nw_path_monitor_create();
  nw_path_monitor_set_queue(monitor_, dispatch_get_main_queue());

  nw_path_monitor_set_update_handler(monitor_, ^(nw_path_t _Nonnull path) {
        bool isOnline = false;
        nw_path_status_t status = nw_path_get_status(path);
        switch (status) {
            case nw_path_status_invalid: {
                qCInfo(LOG_BASIC) << "PathMonitor: network path is invalid";
                break;
            }
            case nw_path_status_satisfied: {
                qCInfo(LOG_BASIC) << "PathMonitor: network is usable";
                isOnline = true;
                break;
            }
            case nw_path_status_satisfiable: {
                qCInfo(LOG_BASIC) << "PathMonitor: network may be usable";
                isOnline = true;
                break;
            }
            case nw_path_status_unsatisfied: {
                qCInfo(LOG_BASIC) << "PathMonitor: network is not usable";
                break;
            }
        }
        if (isOnline) {
            cancelOfflineDebounce();
            if (!curIsOnline_) {
                curIsOnline_ = true;
                emit connectivityStateChanged(true);
            }
        } else if (curIsOnline_ && !debounceTimer_) {
            startOfflineDebounce();
        }
    });
  nw_path_monitor_start(monitor_);
  int g = 0;
}

PathMonitor::~PathMonitor()
{
  nw_path_monitor_cancel(monitor_);
  nw_release(monitor_);
  cancelOfflineDebounce();
}

void PathMonitor::startOfflineDebounce()
{
    debounceTimer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(debounceTimer_, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), DISPATCH_TIME_FOREVER, 0);
    dispatch_source_set_event_handler(debounceTimer_, ^{
        dispatch_source_cancel(debounceTimer_);
        debounceTimer_ = nullptr;
        curIsOnline_ = false;
        qCInfo(LOG_BASIC) << "PathMonitor: no connectivity debounce elapsed, emitting offline";
        emit connectivityStateChanged(false);
    });
    dispatch_resume(debounceTimer_);
}

void PathMonitor::cancelOfflineDebounce()
{
    if (debounceTimer_) {
        dispatch_source_cancel(debounceTimer_);
        debounceTimer_ = nullptr;
    }
}
